#pylint: disable-msg=C0111

"""
Postjob task.

Postjob tasks are responsible for setting the final status of the HQE
and Host, and scheduling additional special agents such as cleanup,
if necessary.
"""

import os

from autotest_lib.client.common_lib.cros.graphite import autotest_stats
from autotest_lib.frontend.afe import models, model_attributes
from autotest_lib.scheduler import agent_task, drones, drone_manager
from autotest_lib.scheduler import email_manager, pidfile_monitor
from autotest_lib.scheduler import scheduler_config
from autotest_lib.server import autoserv_utils


_parser_path = os.path.join(drones.AUTOTEST_INSTALL_DIR, 'tko', 'parse')


class PostJobTask(agent_task.AgentTask):
    def __init__(self, queue_entries, log_file_name):
        super(PostJobTask, self).__init__(log_file_name=log_file_name)

        self.queue_entries = queue_entries

        self._autoserv_monitor = pidfile_monitor.PidfileRunMonitor()
        self._autoserv_monitor.attach_to_existing_process(
                self._working_directory())


    def _command_line(self):
        # Do we need testing_mode?
        return self._generate_command(
                self._drone_manager.absolute_path(self._working_directory()))


    def _generate_command(self, results_dir):
        raise NotImplementedError('Subclasses must override this')


    @property
    def owner_username(self):
        return self.queue_entries[0].job.owner


    def _working_directory(self):
        return self._get_consistent_execution_path(self.queue_entries)


    def _paired_with_monitor(self):
        return self._autoserv_monitor


    def _job_was_aborted(self):
        was_aborted = None
        for queue_entry in self.queue_entries:
            queue_entry.update_from_database()
            if was_aborted is None: # first queue entry
                was_aborted = bool(queue_entry.aborted)
            elif was_aborted != bool(queue_entry.aborted): # subsequent entries
                entries = ['%s (aborted: %s)' % (entry, entry.aborted)
                           for entry in self.queue_entries]
                email_manager.manager.enqueue_notify_email(
                        'Inconsistent abort state',
                        'Queue entries have inconsistent abort state:\n' +
                        '\n'.join(entries))
                # don't crash here, just assume true
                return True
        return was_aborted


    def _final_status(self):
        if self._job_was_aborted():
            return models.HostQueueEntry.Status.ABORTED

        # we'll use a PidfileRunMonitor to read the autoserv exit status
        if self._autoserv_monitor.exit_code() == 0:
            return models.HostQueueEntry.Status.COMPLETED
        return models.HostQueueEntry.Status.FAILED


    def _set_all_statuses(self, status):
        for queue_entry in self.queue_entries:
            queue_entry.set_status(status)


    def abort(self):
        # override AgentTask.abort() to avoid killing the process and ending
        # the task.  post-job tasks continue when the job is aborted.
        pass


    def _pidfile_label(self):
        # '.autoserv_execute' -> 'autoserv'
        return self._pidfile_name()[1:-len('_execute')]


class SelfThrottledPostJobTask(PostJobTask):
    """
    PostJobTask that maintains its own process limit.

    We throttle tasks like parsing because we don't want them to
    hold up tests. At the same time we don't wish to build up load
    that will take forever to parse.
    """
    _num_running_processes = 0
    # Last known limit of max processes, used to check whether
    # max processes config has been changed.
    _last_known_max_processes = 0
    # Whether an email should be sent to notifiy process limit being hit.
    _notification_on = True
    # Once process limit is hit, an email will be sent.
    # To prevent spams, do not send another email until
    # it drops to lower than the following level.
    REVIVE_NOTIFICATION_THRESHOLD = 0.80


    @classmethod
    def _increment_running_processes(cls):
        cls._num_running_processes += 1
        autotest_stats.Gauge('scheduler').send(
                '%s.num_running_processes' % cls.__name__,
                cls._num_running_processes)


    @classmethod
    def _decrement_running_processes(cls):
        cls._num_running_processes -= 1
        autotest_stats.Gauge('scheduler').send(
                '%s.num_running_processes' % cls.__name__,
                cls._num_running_processes)


    @classmethod
    def _max_processes(cls):
        raise NotImplementedError


    @classmethod
    def _can_run_new_process(cls):
        return cls._num_running_processes < cls._max_processes()


    def _process_started(self):
        return bool(self.monitor)


    def tick(self):
        # override tick to keep trying to start until the process count goes
        # down and we can, at which point we revert to default behavior
        if self._process_started():
            super(SelfThrottledPostJobTask, self).tick()
        else:
            self._try_starting_process()


    def run(self):
        # override run() to not actually run unless we can
        self._try_starting_process()


    @classmethod
    def _notify_process_limit_hit(cls):
        """Send an email to notify that process limit is hit."""
        if cls._notification_on:
            subject = '%s: hitting max process limit.' % cls.__name__
            message = ('Running processes/Max processes: %d/%d'
                       % (cls._num_running_processes, cls._max_processes()))
            email_manager.manager.enqueue_notify_email(subject, message)
            cls._notification_on = False


    @classmethod
    def _reset_notification_switch_if_necessary(cls):
        """Reset _notification_on if necessary.

        Set _notification_on to True on the following cases:
        1) If the limit of max processes configuration changes;
        2) If _notification_on is False and the number of running processes
           drops to lower than a level defined in REVIVE_NOTIFICATION_THRESHOLD.

        """
        if cls._last_known_max_processes != cls._max_processes():
            cls._notification_on = True
            cls._last_known_max_processes = cls._max_processes()
            return
        percentage = float(cls._num_running_processes) / cls._max_processes()
        if (not cls._notification_on and
            percentage < cls.REVIVE_NOTIFICATION_THRESHOLD):
            cls._notification_on = True


    def _try_starting_process(self):
        self._reset_notification_switch_if_necessary()
        if not self._can_run_new_process():
            self._notify_process_limit_hit()
            return

        # actually run the command
        super(SelfThrottledPostJobTask, self).run()
        if self._process_started():
            self._increment_running_processes()


    def finished(self, success):
        super(SelfThrottledPostJobTask, self).finished(success)
        if self._process_started():
            self._decrement_running_processes()


class GatherLogsTask(PostJobTask):
    """
    Task responsible for
    * gathering uncollected logs (if Autoserv crashed hard or was killed)
    * copying logs to the results repository
    * spawning CleanupTasks for hosts, if necessary
    * spawning a FinalReparseTask for the job
    * setting the final status of the host, directly or through a cleanup
    """
    def __init__(self, queue_entries, recover_run_monitor=None):
        self._job = queue_entries[0].job
        super(GatherLogsTask, self).__init__(
            queue_entries, log_file_name='.collect_crashinfo.log')
        self._set_ids(queue_entries=queue_entries)


    # TODO: Refactor into autoserv_utils. crbug.com/243090
    def _generate_command(self, results_dir):
        host_list = ','.join(queue_entry.host.hostname
                             for queue_entry in self.queue_entries)
        return [autoserv_utils.autoserv_path , '-p',
                '--pidfile-label=%s' % self._pidfile_label(),
                '--use-existing-results', '--collect-crashinfo',
                '-m', host_list, '-r', results_dir]


    @property
    def num_processes(self):
        return len(self.queue_entries)


    def _pidfile_name(self):
        return drone_manager.CRASHINFO_PID_FILE


    def prolog(self):
        self._check_queue_entry_statuses(
                self.queue_entries,
                allowed_hqe_statuses=(models.HostQueueEntry.Status.GATHERING,),
                allowed_host_statuses=(models.Host.Status.RUNNING,))

        super(GatherLogsTask, self).prolog()


    def epilog(self):
        super(GatherLogsTask, self).epilog()
        self._parse_results(self.queue_entries)
        self._reboot_hosts()


    def _reboot_hosts(self):
        if self._autoserv_monitor.has_process():
            final_success = (self._final_status() ==
                             models.HostQueueEntry.Status.COMPLETED)
            num_tests_failed = self._autoserv_monitor.num_tests_failed()
        else:
            final_success = False
            num_tests_failed = 0
        reboot_after = self._job.reboot_after
        do_reboot = (
                # always reboot after aborted jobs
                self._final_status() == models.HostQueueEntry.Status.ABORTED
                or reboot_after == model_attributes.RebootAfter.ALWAYS
                or (reboot_after == model_attributes.RebootAfter.IF_ALL_TESTS_PASSED
                    and final_success and num_tests_failed == 0)
                or num_tests_failed > 0)

        for queue_entry in self.queue_entries:
            if do_reboot:
                # don't pass the queue entry to the CleanupTask. if the cleanup
                # fails, the job doesn't care -- it's over.
                models.SpecialTask.objects.create(
                        host=models.Host.objects.get(id=queue_entry.host.id),
                        task=models.SpecialTask.Task.CLEANUP,
                        requested_by=self._job.owner_model())
            else:
                queue_entry.host.set_status(models.Host.Status.READY)


    def run(self):
        autoserv_exit_code = self._autoserv_monitor.exit_code()
        # only run if Autoserv exited due to some signal. if we have no exit
        # code, assume something bad (and signal-like) happened.
        if autoserv_exit_code is None or os.WIFSIGNALED(autoserv_exit_code):
            super(GatherLogsTask, self).run()
        else:
            self.finished(True)


class FinalReparseTask(SelfThrottledPostJobTask):
    def __init__(self, queue_entries):
        super(FinalReparseTask, self).__init__(queue_entries,
                                               log_file_name='.parse.log')
        # don't use _set_ids, since we don't want to set the host_ids
        self.queue_entry_ids = [entry.id for entry in queue_entries]


    def _generate_command(self, results_dir):
        return [_parser_path, '--write-pidfile', '--record-duration',
                '-l', '2', '-r', '-o', results_dir]


    @property
    def num_processes(self):
        return 0 # don't include parser processes in accounting


    def _pidfile_name(self):
        return drone_manager.PARSER_PID_FILE


    @classmethod
    def _max_processes(cls):
        return scheduler_config.config.max_parse_processes


    def prolog(self):
        self._check_queue_entry_statuses(
                self.queue_entries,
                allowed_hqe_statuses=(models.HostQueueEntry.Status.PARSING,))

        super(FinalReparseTask, self).prolog()


    def epilog(self):
        super(FinalReparseTask, self).epilog()
        self._archive_results(self.queue_entries)


class ArchiveResultsTask(SelfThrottledPostJobTask):
    _ARCHIVING_FAILED_FILE = '.archiver_failed'

    def __init__(self, queue_entries):
        super(ArchiveResultsTask, self).__init__(queue_entries,
                                                 log_file_name='.archiving.log')
        # don't use _set_ids, since we don't want to set the host_ids
        self.queue_entry_ids = [entry.id for entry in queue_entries]


    def _pidfile_name(self):
        return drone_manager.ARCHIVER_PID_FILE


    # TODO: Refactor into autoserv_utils. crbug.com/243090
    def _generate_command(self, results_dir):
        return [autoserv_utils.autoserv_path , '-p',
                '--pidfile-label=%s' % self._pidfile_label(), '-r', results_dir,
                '--use-existing-results', '--control-filename=control.archive',
                os.path.join(drones.AUTOTEST_INSTALL_DIR, 'scheduler',
                             'archive_results.control.srv')]


    @classmethod
    def _max_processes(cls):
        return scheduler_config.config.max_transfer_processes


    def prolog(self):
        self._check_queue_entry_statuses(
                self.queue_entries,
                allowed_hqe_statuses=(models.HostQueueEntry.Status.ARCHIVING,))

        super(ArchiveResultsTask, self).prolog()


    def epilog(self):
        super(ArchiveResultsTask, self).epilog()
        if not self.success and self._paired_with_monitor().has_process():
            failed_file = os.path.join(self._working_directory(),
                                       self._ARCHIVING_FAILED_FILE)
            paired_process = self._paired_with_monitor().get_process()
            self._drone_manager.write_lines_to_file(
                    failed_file, ['Archiving failed with exit code %s'
                                  % self.monitor.exit_code()],
                    paired_with_process=paired_process)
        self._set_all_statuses(self._final_status())
