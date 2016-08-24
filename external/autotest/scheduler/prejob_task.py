#pylint: disable-msg=C0111

"""
Prejob tasks.

Prejob tasks _usually_ run before a job and verify the state of a machine.
Cleanup and repair are exceptions, cleanup can run after a job too, while
repair will run anytime the host needs a repair, which could be pre or post
job. Most of the work specific to this module is achieved through the prolog
and epilog of each task.

All prejob tasks must have a host, though they may not have an HQE. If a
prejob task has a hqe, it will activate the hqe through its on_pending
method on successful completion. A row in afe_special_tasks with values:
    host=C1, unlocked, is_active=0, is_complete=0, type=Verify
will indicate to the scheduler that it needs to schedule a new special task
of type=Verify, against the C1 host. While the special task is running
the scheduler only monitors it through the Agent, and its is_active bit=1.
Once a special task finishes, we set its is_active=0, is_complete=1 and
success bits, so the scheduler ignores it.
HQE.on_pending:
    Host, HQE -> Pending, Starting
    This status is acted upon in the scheduler, to assign an AgentTask.
PreJobTask:
    epilog:
        failure:
            requeue hqe
            repair the host
Children PreJobTasks:
    prolog:
        set Host, HQE status
    epilog:
        success:
            on_pending
        failure:
            repair throgh PreJobTask
            set Host, HQE status

Failing a prejob task effects both the Host and the HQE, as follows:

- Host: PreJob failure will result in a Repair job getting queued against
the host, is we haven't already tried repairing it more than the
max_repair_limit. When this happens, the host will remain in whatever status
the prejob task left it in, till the Repair job puts it into 'Repairing'. This
way the host_scheduler won't pick bad hosts and assign them to jobs.

If we have already tried repairing the host too many times, the PreJobTask
will flip the host to 'RepairFailed' in its epilog, and it will remain in this
state till it is recovered and reverified.

- HQE: Is either requeued or failed. Requeuing the HQE involves putting it
in the Queued state and setting its host_id to None, so it gets a new host
in the next scheduler tick. Failing the HQE results in either a Parsing
or Archiving postjob task, and an eventual Failed status for the HQE.
"""

import logging
import os

from autotest_lib.client.common_lib import host_protections
from autotest_lib.frontend.afe import models
from autotest_lib.scheduler import agent_task, scheduler_config
from autotest_lib.server import autoserv_utils
from autotest_lib.server.cros import provision


class PreJobTask(agent_task.SpecialAgentTask):
    def _copy_to_results_repository(self):
        if not self.queue_entry or self.queue_entry.meta_host:
            return

        self.queue_entry.set_execution_subdir()
        log_name = os.path.basename(self.task.execution_path())
        source = os.path.join(self.task.execution_path(), 'debug',
                              'autoserv.DEBUG')
        destination = os.path.join(
                self.queue_entry.execution_path(), log_name)

        self.monitor.try_copy_to_results_repository(
                source, destination_path=destination)


    def epilog(self):
        super(PreJobTask, self).epilog()

        if self.success:
            return

        if self.host.protection == host_protections.Protection.DO_NOT_VERIFY:
            # effectively ignore failure for these hosts
            self.success = True
            return

        if self.queue_entry:
            # If we requeue a HQE, we should cancel any remaining pre-job
            # tasks against this host, otherwise we'll be left in a state
            # where a queued HQE has special tasks to run against a host.
            models.SpecialTask.objects.filter(
                    queue_entry__id=self.queue_entry.id,
                    host__id=self.host.id,
                    is_complete=0).update(is_complete=1, success=0)

            previous_provisions = models.SpecialTask.objects.filter(
                    task=models.SpecialTask.Task.PROVISION,
                    queue_entry_id=self.queue_entry.id).count()
            if (previous_provisions >
                scheduler_config.config.max_provision_retries):
                self._actually_fail_queue_entry()
                # This abort will mark the aborted bit on the HQE itself, to
                # signify that we're killing it.  Technically it also will do
                # the recursive aborting of all child jobs, but that shouldn't
                # matter here, as only suites have children, and those are
                # hostless and thus don't have provisioning.
                # TODO(milleral) http://crbug.com/188217
                # However, we can't actually do this yet, as if we set the
                # abort bit the FinalReparseTask will set the status of the HQE
                # to ABORTED, which then means that we don't show the status in
                # run_suite.  So in the meantime, don't mark the HQE as
                # aborted.
                # queue_entry.abort()
            else:
                # requeue() must come after handling provision retries, since
                # _actually_fail_queue_entry needs an execution subdir.
                # We also don't want to requeue if we hit the provision retry
                # limit, since then we overwrite the PARSING state of the HQE.
                self.queue_entry.requeue()

            # Limit the repair on a host when a prejob task fails, e.g., reset,
            # verify etc. The number of repair jobs is limited to the specific
            # HQE and host.
            previous_repairs = models.SpecialTask.objects.filter(
                    task=models.SpecialTask.Task.REPAIR,
                    queue_entry_id=self.queue_entry.id,
                    host_id=self.queue_entry.host_id).count()
            if previous_repairs >= scheduler_config.config.max_repair_limit:
                self.host.set_status(models.Host.Status.REPAIR_FAILED)
                self._fail_queue_entry()
                return

            queue_entry = models.HostQueueEntry.objects.get(
                    id=self.queue_entry.id)
        else:
            queue_entry = None

        models.SpecialTask.objects.create(
                host=models.Host.objects.get(id=self.host.id),
                task=models.SpecialTask.Task.REPAIR,
                queue_entry=queue_entry,
                requested_by=self.task.requested_by)


    def _should_pending(self):
        """
        Decide if we should call the host queue entry's on_pending method.
        We should if:
        1) There exists an associated host queue entry.
        2) The current special task completed successfully.
        3) There do not exist any more special tasks to be run before the
           host queue entry starts.

        @returns: True if we should call pending, false if not.

        """
        if not self.queue_entry or not self.success:
            return False

        # We know if this is the last one when we create it, so we could add
        # another column to the database to keep track of this information, but
        # I expect the overhead of querying here to be minimal.
        queue_entry = models.HostQueueEntry.objects.get(id=self.queue_entry.id)
        queued = models.SpecialTask.objects.filter(
                host__id=self.host.id, is_active=False,
                is_complete=False, queue_entry=queue_entry)
        queued = queued.exclude(id=self.task.id)
        return queued.count() == 0


class VerifyTask(PreJobTask):
    TASK_TYPE = models.SpecialTask.Task.VERIFY


    def __init__(self, task):
        args = ['-v']
        if task.queue_entry:
            args.extend(self._generate_autoserv_label_args(task))
        super(VerifyTask, self).__init__(task, args)
        self._set_ids(host=self.host, queue_entries=[self.queue_entry])


    def prolog(self):
        super(VerifyTask, self).prolog()

        logging.info("starting verify on %s", self.host.hostname)
        if self.queue_entry:
            self.queue_entry.set_status(models.HostQueueEntry.Status.VERIFYING)
        self.host.set_status(models.Host.Status.VERIFYING)

        # Delete any queued manual reverifies for this host.  One verify will do
        # and there's no need to keep records of other requests.
        self.remove_special_tasks(models.SpecialTask.Task.VERIFY,
                                  keep_last_one=True)


    def epilog(self):
        super(VerifyTask, self).epilog()
        if self.success:
            if self._should_pending():
                self.queue_entry.on_pending()
            else:
                self.host.set_status(models.Host.Status.READY)


class CleanupTask(PreJobTask):
    # note this can also run post-job, but when it does, it's running standalone
    # against the host (not related to the job), so it's not considered a
    # PostJobTask

    TASK_TYPE = models.SpecialTask.Task.CLEANUP


    def __init__(self, task, recover_run_monitor=None):
        args = ['--cleanup']
        if task.queue_entry:
            args.extend(self._generate_autoserv_label_args(task))
        super(CleanupTask, self).__init__(task, args)
        self._set_ids(host=self.host, queue_entries=[self.queue_entry])


    def prolog(self):
        super(CleanupTask, self).prolog()
        logging.info("starting cleanup task for host: %s", self.host.hostname)
        self.host.set_status(models.Host.Status.CLEANING)
        if self.queue_entry:
            self.queue_entry.set_status(models.HostQueueEntry.Status.CLEANING)


    def _finish_epilog(self):
        if not self.queue_entry or not self.success:
            return

        do_not_verify_protection = host_protections.Protection.DO_NOT_VERIFY
        should_run_verify = (
                self.queue_entry.job.run_verify
                and self.host.protection != do_not_verify_protection)
        if should_run_verify:
            entry = models.HostQueueEntry.objects.get(id=self.queue_entry.id)
            models.SpecialTask.objects.create(
                    host=models.Host.objects.get(id=self.host.id),
                    queue_entry=entry,
                    task=models.SpecialTask.Task.VERIFY)
        else:
            if self._should_pending():
                self.queue_entry.on_pending()


    def epilog(self):
        super(CleanupTask, self).epilog()

        if self.success:
            self.host.update_field('dirty', 0)
            self.host.set_status(models.Host.Status.READY)

        self._finish_epilog()


class ResetTask(PreJobTask):
    """Task to reset a DUT, including cleanup and verify."""
    # note this can also run post-job, but when it does, it's running standalone
    # against the host (not related to the job), so it's not considered a
    # PostJobTask

    TASK_TYPE = models.SpecialTask.Task.RESET


    def __init__(self, task, recover_run_monitor=None):
        args = ['--reset']
        if task.queue_entry:
            args.extend(self._generate_autoserv_label_args(task))
        super(ResetTask, self).__init__(task, args)
        self._set_ids(host=self.host, queue_entries=[self.queue_entry])


    def prolog(self):
        super(ResetTask, self).prolog()
        logging.info('starting reset task for host: %s',
                     self.host.hostname)
        self.host.set_status(models.Host.Status.RESETTING)
        if self.queue_entry:
            self.queue_entry.set_status(models.HostQueueEntry.Status.RESETTING)

        # Delete any queued cleanups for this host.
        self.remove_special_tasks(models.SpecialTask.Task.CLEANUP,
                                  keep_last_one=False)

        # Delete any queued reverifies for this host.
        self.remove_special_tasks(models.SpecialTask.Task.VERIFY,
                                  keep_last_one=False)

        # Only one reset is needed.
        self.remove_special_tasks(models.SpecialTask.Task.RESET,
                                  keep_last_one=True)


    def epilog(self):
        super(ResetTask, self).epilog()

        if self.success:
            self.host.update_field('dirty', 0)

            if self._should_pending():
                self.queue_entry.on_pending()
            else:
                self.host.set_status(models.Host.Status.READY)


class ProvisionTask(PreJobTask):
    TASK_TYPE = models.SpecialTask.Task.PROVISION

    def __init__(self, task):
        # Provisioning requires that we be associated with a job/queue entry
        assert task.queue_entry, "No HQE associated with provision task!"
        # task.queue_entry is an afe model HostQueueEntry object.
        # self.queue_entry is a scheduler models HostQueueEntry object, but
        # it gets constructed and assigned in __init__, so it's not available
        # yet.  Therefore, we're stuck pulling labels off of the afe model
        # so that we can pass the --provision args into the __init__ call.
        labels = {x.name for x in task.queue_entry.job.labels}
        _, provisionable = provision.filter_labels(labels)
        extra_command_args = ['--provision',
                              '--job-labels', ','.join(provisionable)]
        super(ProvisionTask, self).__init__(task, extra_command_args)
        self._set_ids(host=self.host, queue_entries=[self.queue_entry])


    def _command_line(self):
        # If we give queue_entry to _autoserv_command_line, then it will append
        # -c for this invocation if the queue_entry is a client side test. We
        # don't want that, as it messes with provisioning, so we just drop it
        # from the arguments here.
        # Note that we also don't verify job_repo_url as provisioining tasks are
        # required to stage whatever content we need, and the job itself will
        # force autotest to be staged if it isn't already.
        return autoserv_utils._autoserv_command_line(self.host.hostname,
                                                     self._extra_command_args,
                                                     in_lab=True)


    def prolog(self):
        super(ProvisionTask, self).prolog()
        # add check for previous provision task and abort if exist.
        logging.info("starting provision task for host: %s", self.host.hostname)
        self.queue_entry.set_status(
                models.HostQueueEntry.Status.PROVISIONING)
        self.host.set_status(models.Host.Status.PROVISIONING)


    def epilog(self):
        super(ProvisionTask, self).epilog()

        # If we were not successful in provisioning the machine
        # leave the DUT in whatever status was set in the PreJobTask's
        # epilog. If this task was successful the host status will get
        # set appropriately as a fallout of the hqe's on_pending. If
        # we don't call on_pending, it can only be because:
        #   1. This task was not successful:
        #       a. Another repair is queued: this repair job will set the host
        #       status, and it will remain in 'Provisioning' till then.
        #       b. We have hit the max_repair_limit: in which case the host
        #       status is set to 'RepairFailed' in the epilog of PreJobTask.
        #   2. The task was successful, but there are other special tasks:
        #      Those special tasks will set the host status appropriately.
        if self._should_pending():
            self.queue_entry.on_pending()


class RepairTask(agent_task.SpecialAgentTask):
    TASK_TYPE = models.SpecialTask.Task.REPAIR


    def __init__(self, task):
        """\
        queue_entry: queue entry to mark failed if this repair fails.
        """
        protection = host_protections.Protection.get_string(
                task.host.protection)
        # normalize the protection name
        protection = host_protections.Protection.get_attr_name(protection)

        args = ['-R', '--host-protection', protection]
        if task.queue_entry:
            args.extend(self._generate_autoserv_label_args(task))

        super(RepairTask, self).__init__(task, args)

        # *don't* include the queue entry in IDs -- if the queue entry is
        # aborted, we want to leave the repair task running
        self._set_ids(host=self.host)


    def prolog(self):
        super(RepairTask, self).prolog()
        logging.info("repair_task starting")
        self.host.set_status(models.Host.Status.REPAIRING)


    def epilog(self):
        super(RepairTask, self).epilog()

        if self.success:
            self.host.set_status(models.Host.Status.READY)
        else:
            self.host.set_status(models.Host.Status.REPAIR_FAILED)
            if self.queue_entry:
                self._fail_queue_entry()
