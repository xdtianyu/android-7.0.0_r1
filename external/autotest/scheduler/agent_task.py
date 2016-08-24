#pylint: disable-msg=C0111

""" This is the module for everything related to the AgentTask.

The BaseAgentTask imposes an interface through which the scheduler can monitor
a processes; Examples of such processes include Verify, Cleanup and the Queue
Tasks that run the tests. The scheduler itself only understands Agents.
Agents:
    The Agent is the bridge between the scheduler and the AgentTask. The
    schedulers tick has a method called handle_agents, which calls the
    tick of each agent in the Dispatchers queue. This leads to the Agent
    polling its AgentTask. The scheduler will keep polling a task through
    the associated Agent till the Agent is removed from the dispatcher.

    At a high level:
        agents finished = tasks done
        agent polls till finished
            task polls till done
                task sets done
        agent is removed from dispatcher
AgentTasks:
    Basic AgentTasks are created when an hqe changes state. Examples of these
    are the QueueTask, which is created when a hqe goes into the Starting state
    and the FinalReparseTask, which is created when the hqe goes into parsing.
SpecialAgentTasks:
    Unlike AgentTasks, SpecialAgentTasks are only created when a row is inserted
    in the afe_special_tasks table. All PrejobTasks are SpecialAgentTasks.

Monitor_db.get_agent_task_for_special_task/get_agent_task_for_queue_entry maps
an AgentTask to an Agent, which the scheduler understands. From this point
onward, the scheduler manages the task through the Agents interface,as follows:
At a high level:
    task poll
        start
            prolog
        tick till we get an exit code
        finished(exit==0)
            done=True
            epilog
                cleanup
                    set is_active, is_complete, success (checked in scheduler)

The first special task for an HQE is usually Reset.
-poll: The first poll will start the task, polls thereafter will call the tasks
       tick method. A started task will have the started bit set.
- start: Call prolog, run the process and set the start bit.
    - prolog: Usually where one puts any model state changes that happen before
              the actual task. Different per Task. Examples of things that might
              happen in a prolog:
                  - state of Host, HQE (to something like Resetting)
                  - delete any unwanted queued special tasks
                  - register a pidfile
                  - set the is_active bit on the special task
    - run:
        - create a PidfileRunMonitor
        - pass the autoserv command, working directory etc to drone manager.
          This will start the actual autoserv process.
   - set the start bit: so subsequent polls do not 'start' again

- tick: For as long as a started tasks done bit is not set, a poll will lead
        to a tick. The tick monitors the pid file of the autoserv process
        running on the drone through the PidfileRunMonitor created in prolog.
        If the autoserv process has finished we call finished with true/false
        depending on autoserv exit code.

        - finished: sets the done and success values, then calls epilog. The
                    done bit is important because the Agent polls this bit to
                    measure the success or failure of its task.

            - epilog: Is generally where we set status of the Host/HQE again,
                      requeue any other task that needs to run after this one
                      and perform cleanup. Just like the prolog, this step is
                      different per task.

                      - cleanup: Sets the is_active and is_complete and success
                                 states on the tasks model. Also uses the
                                 drone_manager to:
                                    unregister the pidfile
                                    copy results of the task
                                 (Note this is not to be confused with the
                                  special task called cleanup).

                      The actions we take in the epilog are based on the
                      success/failure of the autoserv process set in cleanup,
                      eg: if reset failed we will enqueue a repair, but if all
                      is well the epilog will just return. Prejob task epilogs
                      also have an on_pending method that change the status of
                      the HQE to pending/starting, which gets picked up in the
                      scheduler.
By this point the is_done flag is set, which results in the Agent noticing that
the task has finished and unregistering it from the dispatcher.Class hierarchy:
BaseAgentTask
 |--->SpecialAgentTask (prejob_task.py)
      |--->RepairTask
      |--->PreJobTask
           |--->Verify, Cleanup, Reset, Provision

 |--->AbstractQueueTask (monitor_db.py)
      |--->QueueTask
      |--->HostlessQueueTask

 |--->PostJobTask (postjob_task.py)
      |--->GatherLogsTask
      |--->SelfThrottledPostJobTask
            |--->FinalReparseTask
            |--->ArchiveResultsTask

"""

import logging
import os
import urllib
import time

from autotest_lib.client.common_lib import global_config
from autotest_lib.client.common_lib import utils
from autotest_lib.client.common_lib.cros.graphite import autotest_stats
from autotest_lib.frontend.afe import models
from autotest_lib.scheduler import drone_manager, pidfile_monitor
from autotest_lib.scheduler import scheduler_lib
from autotest_lib.scheduler import rdb_lib
from autotest_lib.scheduler import scheduler_models
from autotest_lib.server import autoserv_utils
from autotest_lib.server import system_utils

CONFIG = global_config.global_config
AUTOSERV_NICE_LEVEL = 10

ENABLE_DRONE_IN_RESTRICTED_SUBNET = CONFIG.get_config_value(
        'CROS', 'enable_drone_in_restricted_subnet', type=bool,
        default=False)


class BaseAgentTask(object):
    class _NullMonitor(object):
        pidfile_id = None

        def has_process(self):
            return True


    def __init__(self, log_file_name=None):
        """
        @param log_file_name: (optional) name of file to log command output to
        """
        self._drone_manager = drone_manager.instance()
        self.done = False
        self.started = False
        self.success = None
        self.aborted = False
        self.monitor = None
        self.queue_entry_ids = []
        self.host_ids = []
        # A map between host id and hostname.
        self.hostnames = {}
        self._log_file_name = log_file_name


    def _set_ids(self, host=None, queue_entries=None):
        if queue_entries and queue_entries != [None]:
            self.host_ids = [entry.host.id for entry in queue_entries]
            self.queue_entry_ids = [entry.id for entry in queue_entries]
            self.hostnames = dict((entry.host.id, entry.host.hostname)
                                  for entry in queue_entries)
        else:
            assert host
            self.host_ids = [host.id]
            self.hostnames = {host.id: host.hostname}


    def poll(self):
        if not self.started:
            self.start()
        if not self.done:
            self.tick()


    def tick(self):
        assert self.monitor
        exit_code = self.monitor.exit_code()
        if exit_code is None:
            return

        success = (exit_code == 0)
        self.finished(success)


    def is_done(self):
        return self.done


    def finished(self, success):
        if self.done:
            assert self.started
            return
        self.started = True
        self.done = True
        self.success = success
        self.epilog()


    def prolog(self):
        """
        To be overridden.
        """
        assert not self.monitor
        self.register_necessary_pidfiles()


    def _log_file(self):
        if not self._log_file_name:
            return None
        return os.path.join(self._working_directory(), self._log_file_name)


    def cleanup(self):
        log_file = self._log_file()
        if self.monitor and log_file:
            self.monitor.try_copy_to_results_repository(log_file)


    def epilog(self):
        """
        To be overridden.
        """
        self.cleanup()
        logging.info("%s finished with success=%s", type(self).__name__,
                     self.success)


    def start(self):
        if not self.started:
            self.prolog()
            self.run()

        self.started = True


    def abort(self):
        if self.monitor:
            self.monitor.kill()
        self.done = True
        self.aborted = True
        self.cleanup()


    def _get_consistent_execution_path(self, execution_entries):
        first_execution_path = execution_entries[0].execution_path()
        for execution_entry in execution_entries[1:]:
            assert execution_entry.execution_path() == first_execution_path, (
                '%s (%s) != %s (%s)' % (execution_entry.execution_path(),
                                        execution_entry,
                                        first_execution_path,
                                        execution_entries[0]))
        return first_execution_path


    def _copy_results(self, execution_entries, use_monitor=None):
        """
        @param execution_entries: list of objects with execution_path() method
        """
        if use_monitor is not None and not use_monitor.has_process():
            return

        assert len(execution_entries) > 0
        if use_monitor is None:
            assert self.monitor
            use_monitor = self.monitor
        assert use_monitor.has_process()
        execution_path = self._get_consistent_execution_path(execution_entries)
        results_path = execution_path + '/'
        use_monitor.try_copy_to_results_repository(results_path)


    def _parse_results(self, queue_entries):
        for queue_entry in queue_entries:
            queue_entry.set_status(models.HostQueueEntry.Status.PARSING)


    def _archive_results(self, queue_entries):
        for queue_entry in queue_entries:
            queue_entry.set_status(models.HostQueueEntry.Status.ARCHIVING)


    def _command_line(self):
        """
        Return the command line to run.  Must be overridden.
        """
        raise NotImplementedError


    @property
    def num_processes(self):
        """
        Return the number of processes forked by this BaseAgentTask's process.
        It may only be approximate.  To be overridden if necessary.
        """
        return 1


    def _paired_with_monitor(self):
        """
        If this BaseAgentTask's process must run on the same machine as some
        previous process, this method should be overridden to return a
        PidfileRunMonitor for that process.
        """
        return self._NullMonitor()


    @property
    def owner_username(self):
        """
        Return login of user responsible for this task.  May be None.  Must be
        overridden.
        """
        raise NotImplementedError


    def _working_directory(self):
        """
        Return the directory where this BaseAgentTask's process executes.
        Must be overridden.
        """
        raise NotImplementedError


    def _pidfile_name(self):
        """
        Return the name of the pidfile this BaseAgentTask's process uses.  To be
        overridden if necessary.
        """
        return drone_manager.AUTOSERV_PID_FILE


    def _check_paired_results_exist(self):
        if not self._paired_with_monitor().has_process():
            metadata = {
                    '_type': 'scheduler_error',
                    'error': 'No paired results in task',
                    'task': str(self),
                    'pidfile_id': str(self._paired_with_monitor().pidfile_id)}
            autotest_stats.Counter('no_paired_results_in_task',
                                   metadata=metadata).increment()
            self.finished(False)
            return False
        return True


    def _create_monitor(self):
        assert not self.monitor
        self.monitor = pidfile_monitor.PidfileRunMonitor()


    def run(self):
        if not self._check_paired_results_exist():
            return

        self._create_monitor()
        self.monitor.run(
                self._command_line(), self._working_directory(),
                num_processes=self.num_processes,
                nice_level=AUTOSERV_NICE_LEVEL, log_file=self._log_file(),
                pidfile_name=self._pidfile_name(),
                paired_with_pidfile=self._paired_with_monitor().pidfile_id,
                username=self.owner_username,
                drone_hostnames_allowed=self.get_drone_hostnames_allowed())


    def get_drone_hostnames_allowed(
            self, restricted_subnets=utils.RESTRICTED_SUBNETS,
            enable_drone_in_subnet=ENABLE_DRONE_IN_RESTRICTED_SUBNET):
        filtered_drones = None
        has_unrestricted_host = False
        if (self.hostnames and restricted_subnets and enable_drone_in_subnet):
            for hostname in self.hostnames.values():
                subnet = utils.get_restricted_subnet(hostname,
                                                     restricted_subnets)

                # Return an empty set if the list of hosts exists both in
                # restricted and unrestricted subnet. No drone can work in such
                # case.
                if ((not subnet and filtered_drones is not None) or
                    (subnet and has_unrestricted_host)):
                    logging.error('The test has some DUT in restricted subnet, '
                                  'but some in unrestricted subnet. Therefore, '
                                  'no drone is available to run the test.')
                    return set()

                if not subnet:
                    has_unrestricted_host = True
                    continue

                server_ip_map=system_utils.DroneCache.get_drone_ip_map()
                filtered_drones_for_host = set(
                        utils.get_servers_in_same_subnet(
                                subnet[0], subnet[1],
                                server_ip_map=server_ip_map))
                logging.info('DUT %s is in restricted subnet, drone can only '
                             'be chosen from %s', hostname,
                             filtered_drones_for_host)
                if filtered_drones is None:
                    filtered_drones = filtered_drones_for_host
                else:
                    filtered_drones = set.intersection(
                            filtered_drones, filtered_drones_for_host)

                # If filtered_drones is an empty set, that means no drone is
                # allowed to run the task. This is different fron None, which
                # means all drones are allowed.
                if filtered_drones == set():
                    logging.error('DUT(s) is in restricted subnet, but no '
                                  'drone is available to run the test.')
                    return filtered_drones

        # If host is not in restricted subnet, use the unrestricted drones only.
        if (filtered_drones is None and restricted_subnets and
            enable_drone_in_subnet):
            filtered_drones = set(
                    system_utils.DroneCache.get_unrestricted_drones(
                            restricted_subnets=restricted_subnets))

        if not models.DroneSet.drone_sets_enabled():
            return filtered_drones

        hqes = models.HostQueueEntry.objects.filter(id__in=self.queue_entry_ids)
        if not hqes:
            # Only special tasks could be missing host queue entries
            assert isinstance(self, SpecialAgentTask)
            return self._user_or_global_default_drone_set(
                    self.task, self.task.requested_by)

        job_ids = hqes.values_list('job', flat=True).distinct()
        assert job_ids.count() == 1, ("BaseAgentTask's queue entries "
                                      "span multiple jobs")

        job = models.Job.objects.get(id=job_ids[0])
        drone_set = job.drone_set
        if not drone_set:
            return self._user_or_global_default_drone_set(job, job.user())

        if filtered_drones:
            return set.intersection(filtered_drones,
                                    drone_set.get_drone_hostnames())
        else:
            return drone_set.get_drone_hostnames()


    def _user_or_global_default_drone_set(self, obj_with_owner, user):
        """
        Returns the user's default drone set, if present.

        Otherwise, returns the global default drone set.
        """
        default_hostnames = models.DroneSet.get_default().get_drone_hostnames()
        if not user:
            logging.warning('%s had no owner; using default drone set',
                         obj_with_owner)
            return default_hostnames
        if not user.drone_set:
            logging.warning('User %s has no default drone set, using global '
                         'default', user.login)
            return default_hostnames
        return user.drone_set.get_drone_hostnames()


    def register_necessary_pidfiles(self):
        pidfile_id = self._drone_manager.get_pidfile_id_from(
                self._working_directory(), self._pidfile_name())
        self._drone_manager.register_pidfile(pidfile_id)

        paired_pidfile_id = self._paired_with_monitor().pidfile_id
        if paired_pidfile_id:
            self._drone_manager.register_pidfile(paired_pidfile_id)


    def recover(self):
        if not self._check_paired_results_exist():
            return

        self._create_monitor()
        self.monitor.attach_to_existing_process(
                self._working_directory(), pidfile_name=self._pidfile_name(),
                num_processes=self.num_processes)
        if not self.monitor.has_process():
            # no process to recover; wait to be started normally
            self.monitor = None
            return

        self.started = True
        logging.info('Recovering process %s for %s at %s',
                     self.monitor.get_process(), type(self).__name__,
                     self._working_directory())


    def _check_queue_entry_statuses(self, queue_entries, allowed_hqe_statuses,
                                    allowed_host_statuses=None):
        class_name = self.__class__.__name__
        for entry in queue_entries:
            if entry.status not in allowed_hqe_statuses:
                raise scheduler_lib.SchedulerError(
                        '%s attempting to start entry with invalid status %s: '
                        '%s' % (class_name, entry.status, entry))
            invalid_host_status = (
                    allowed_host_statuses is not None
                    and entry.host.status not in allowed_host_statuses)
            if invalid_host_status:
                raise scheduler_lib.SchedulerError(
                        '%s attempting to start on queue entry with invalid '
                        'host status %s: %s'
                        % (class_name, entry.host.status, entry))


SiteAgentTask = utils.import_site_class(
    __file__, 'autotest_lib.scheduler.site_monitor_db',
    'SiteAgentTask', BaseAgentTask)

class AgentTask(SiteAgentTask):
    pass


class TaskWithJobKeyvals(object):
    """AgentTask mixin providing functionality to help with job keyval files."""
    _KEYVAL_FILE = 'keyval'
    def _format_keyval(self, key, value):
        return '%s=%s' % (key, value)


    def _keyval_path(self):
        """Subclasses must override this"""
        raise NotImplementedError


    def _write_keyval_after_job(self, field, value):
        assert self.monitor
        if not self.monitor.has_process():
            return
        self._drone_manager.write_lines_to_file(
            self._keyval_path(), [self._format_keyval(field, value)],
            paired_with_process=self.monitor.get_process())


    def _job_queued_keyval(self, job):
        return 'job_queued', int(time.mktime(job.created_on.timetuple()))


    def _write_job_finished(self):
        self._write_keyval_after_job("job_finished", int(time.time()))


    def _write_keyvals_before_job_helper(self, keyval_dict, keyval_path):
        keyval_contents = '\n'.join(self._format_keyval(key, value)
                                    for key, value in keyval_dict.iteritems())
        # always end with a newline to allow additional keyvals to be written
        keyval_contents += '\n'
        self._drone_manager.attach_file_to_execution(self._working_directory(),
                                                keyval_contents,
                                                file_path=keyval_path)


    def _write_keyvals_before_job(self, keyval_dict):
        self._write_keyvals_before_job_helper(keyval_dict, self._keyval_path())


    def _write_host_keyvals(self, host):
        keyval_path = os.path.join(self._working_directory(), 'host_keyvals',
                                   host.hostname)
        platform, all_labels = host.platform_and_labels()
        all_labels = [ urllib.quote(label) for label in all_labels ]
        keyval_dict = dict(platform=platform, labels=','.join(all_labels))
        self._write_keyvals_before_job_helper(keyval_dict, keyval_path)


class SpecialAgentTask(AgentTask, TaskWithJobKeyvals):
    """
    Subclass for AgentTasks that correspond to a SpecialTask entry in the DB.
    """

    TASK_TYPE = None
    host = None
    queue_entry = None

    def __init__(self, task, extra_command_args):
        super(SpecialAgentTask, self).__init__()

        assert self.TASK_TYPE is not None, 'self.TASK_TYPE must be overridden'

        self.host = rdb_lib.get_hosts([task.host.id])[0]
        self.host.dbg_str = 'Task: %s' % str(task)
        self.queue_entry = None
        if task.queue_entry:
            self.queue_entry = scheduler_models.HostQueueEntry(
                    id=task.queue_entry.id)
            self.host.dbg_str += self.queue_entry.get_dbg_str()

        self.task = task
        self._extra_command_args = extra_command_args
        self.host.metadata = self.get_metadata()


    def get_metadata(self):
        """Get a dictionary that contains task information.

        The return value is a dictionary that includes task information like id,
        name and related job information. The value will be stored in metadata
        database.
        @return: A dictionary containing the task id, name and related job id.
                 If some attributes are failed to be accessed, an empty
                 dictionary will be returned, and error will be logged.
        """
        try:
            metadata = {'task_id':self.task.id, 'task_name':self.task.task,
                        'hostname':self.task.host.hostname}
            if self.task.queue_entry:
                job = self.task.queue_entry.job
                metadata.update(
                        scheduler_models.get_job_metadata(job))
            return metadata
        except AttributeError as e:
            logging.error('Task has missing attribute: %s', e)
            return {}


    def _keyval_path(self):
        return os.path.join(self._working_directory(), self._KEYVAL_FILE)


    def _command_line(self):
        return autoserv_utils._autoserv_command_line(self.host.hostname,
                                                     self._extra_command_args,
                                                     queue_entry=self.queue_entry,
                                                     in_lab=True)


    def _working_directory(self):
        return self.task.execution_path()


    @property
    def owner_username(self):
        if self.task.requested_by:
            return self.task.requested_by.login
        return None


    def prolog(self):
        super(SpecialAgentTask, self).prolog()
        self.task.activate()
        self._write_host_keyvals(self.host)


    def _fail_queue_entry(self):
        assert self.queue_entry

        if self.queue_entry.meta_host:
            return # don't fail metahost entries, they'll be reassigned

        self.queue_entry.update_from_database()
        if self.queue_entry.status != models.HostQueueEntry.Status.QUEUED:
            return # entry has been aborted

        self._actually_fail_queue_entry()


    # TODO(milleral): http://crbug.com/268607
    # All this used to be a part of _fail_queue_entry.  The
    # exact semantics of when one should and should not be failing a queue
    # entry need to be worked out, because provisioning has placed us in a
    # case where we want to fail a queue entry that could be requeued,
    # which makes us fail the two above if statements, and thus
    # _fail_queue_entry() would exit early and have no effect.
    # What's left here with _actually_fail_queue_entry is a hack to be able to
    # bypass the checks and unconditionally execute the code.
    def _actually_fail_queue_entry(self):
        self.queue_entry.set_execution_subdir()
        queued_key, queued_time = self._job_queued_keyval(
            self.queue_entry.job)
        self._write_keyval_after_job(queued_key, queued_time)
        self._write_job_finished()

        # copy results logs into the normal place for job results
        self.monitor.try_copy_results_on_drone(
                source_path=self._working_directory() + '/',
                destination_path=self.queue_entry.execution_path() + '/')

        pidfile_id = self._drone_manager.get_pidfile_id_from(
                self.queue_entry.execution_path(),
                pidfile_name=drone_manager.AUTOSERV_PID_FILE)
        self._drone_manager.register_pidfile(pidfile_id)

        if self.queue_entry.job.parse_failed_repair:
            self._parse_results([self.queue_entry])
        else:
            self._archive_results([self.queue_entry])

        # Also fail all other special tasks that have not yet run for this HQE
        pending_tasks = models.SpecialTask.objects.filter(
                queue_entry__id=self.queue_entry.id,
                is_complete=0)
        for task in pending_tasks:
            task.finish(False)


    def cleanup(self):
        super(SpecialAgentTask, self).cleanup()

        # We will consider an aborted task to be "Failed"
        self.task.finish(bool(self.success))

        if self.monitor:
            if self.monitor.has_process():
                self._copy_results([self.task])
            if self.monitor.pidfile_id is not None:
                self._drone_manager.unregister_pidfile(self.monitor.pidfile_id)


    def remove_special_tasks(self, special_task_to_remove, keep_last_one=False):
        """Remove a type of special task in all tasks, keep last one if needed.

        @param special_task_to_remove: type of special task to be removed, e.g.,
            models.SpecialTask.Task.VERIFY.
        @param keep_last_one: True to keep the last special task if its type is
            the same as of special_task_to_remove.

        """
        queued_special_tasks = models.SpecialTask.objects.filter(
            host__id=self.host.id,
            task=special_task_to_remove,
            is_active=False, is_complete=False, queue_entry=None)
        if keep_last_one:
            queued_special_tasks = queued_special_tasks.exclude(id=self.task.id)
        queued_special_tasks.delete()


    def _generate_autoserv_label_args(self, task):
        """
        @param task: An instance of afe model's SpecialTask.
        @returns: The list of arguments to pass to autoserv to tell it what the
                  labels of a job are.

        """
        labels = {x.name for x in task.queue_entry.job.labels}
        return ['--job-labels', ','.join(labels)]
