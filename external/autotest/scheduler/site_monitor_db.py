# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

#pylint: disable-msg=C0111

import os
import logging
import time

from autotest_lib.client.common_lib import global_config
from autotest_lib.client.common_lib.cros.graphite import autotest_stats
from autotest_lib.frontend.afe import models
from autotest_lib.scheduler import email_manager
from autotest_lib.scheduler import scheduler_config, scheduler_models


# Override default parser with our site parser.
def parser_path(install_dir):
    """Return site implementation of parser.

    @param install_dir: installation directory.
    """
    return os.path.join(install_dir, 'tko', 'site_parse')


class SiteAgentTask(object):
    """
    SiteAgentTask subclasses BaseAgentTask in monitor_db.
    """


    def _archive_results(self, queue_entries):
        """
        Set the status of queue_entries to ARCHIVING.

        This method sets the status of the queue_entries to ARCHIVING
        if the enable_archiving flag is true in global_config.ini.
        Otherwise, it bypasses the archiving step and sets the queue entries
        to the final status of current step.
        """
        enable_archiving = global_config.global_config.get_config_value(
            scheduler_config.CONFIG_SECTION, 'enable_archiving', type=bool)
        # Set the status of the queue entries to archiving or self final status
        if enable_archiving:
            status = models.HostQueueEntry.Status.ARCHIVING
        else:
            status = self._final_status()

        for queue_entry in self.queue_entries:
            queue_entry.set_status(status)


    def _check_queue_entry_statuses(self, queue_entries, allowed_hqe_statuses,
                                    allowed_host_statuses=None):
        """
        Forked from monitor_db.py
        """
        class_name = self.__class__.__name__
        for entry in queue_entries:
            if entry.status not in allowed_hqe_statuses:
                # In the orignal code, here we raise an exception. In an
                # effort to prevent downtime we will instead abort the job and
                # send out an email notifying us this has occured.
                error_message = ('%s attempting to start entry with invalid '
                                 'status %s: %s. Aborting Job: %s.'
                                 % (class_name, entry.status, entry,
                                    entry.job))
                logging.error(error_message)
                email_manager.manager.enqueue_notify_email(
                    'Job Aborted - Invalid Host Queue Entry Status',
                    error_message)
                entry.job.request_abort()
            invalid_host_status = (
                    allowed_host_statuses is not None
                    and entry.host.status not in allowed_host_statuses)
            if invalid_host_status:
                # In the orignal code, here we raise an exception. In an
                # effort to prevent downtime we will instead abort the job and
                # send out an email notifying us this has occured.
                error_message = ('%s attempting to start on queue entry with '
                                 'invalid host status %s: %s. Aborting Job: %s'
                                 % (class_name, entry.host.status, entry,
                                    entry.job))
                logging.error(error_message)
                email_manager.manager.enqueue_notify_email(
                    'Job Aborted - Invalid Host Status', error_message)
                entry.job.request_abort()


class SiteDispatcher(object):
    """
    SiteDispatcher subclasses BaseDispatcher in monitor_db.
    """
    DEFAULT_REQUESTED_BY_USER_ID = 1


    _timer = autotest_stats.Timer('scheduler')
    _gauge = autotest_stats.Gauge('scheduler_rel')
    _tick_start = None


    @_timer.decorate
    def tick(self):
        self._tick_start = time.time()
        super(SiteDispatcher, self).tick()
        self._gauge.send('tick', time.time() - self._tick_start)

    @_timer.decorate
    def _garbage_collection(self):
        super(SiteDispatcher, self)._garbage_collection()
        if self._tick_start:
            self._gauge.send('_garbage_collection',
                             time.time() - self._tick_start)

    @_timer.decorate
    def _run_cleanup(self):
        super(SiteDispatcher, self)._run_cleanup()
        if self._tick_start:
            self._gauge.send('_run_cleanup', time.time() - self._tick_start)

    @_timer.decorate
    def _find_aborting(self):
        super(SiteDispatcher, self)._find_aborting()
        if self._tick_start:
            self._gauge.send('_find_aborting', time.time() - self._tick_start)

    @_timer.decorate
    def _process_recurring_runs(self):
        super(SiteDispatcher, self)._process_recurring_runs()
        if self._tick_start:
            self._gauge.send('_process_recurring_runs',
                             time.time() - self._tick_start)

    @_timer.decorate
    def _schedule_delay_tasks(self):
        super(SiteDispatcher, self)._schedule_delay_tasks()
        if self._tick_start:
            self._gauge.send('_schedule_delay_tasks',
                             time.time() - self._tick_start)

    @_timer.decorate
    def _schedule_running_host_queue_entries(self):
        super(SiteDispatcher, self)._schedule_running_host_queue_entries()
        if self._tick_start:
            self._gauge.send('_schedule_running_host_queue_entries',
                             time.time() - self._tick_start)

    @_timer.decorate
    def _schedule_special_tasks(self):
        super(SiteDispatcher, self)._schedule_special_tasks()
        if self._tick_start:
            self._gauge.send('_schedule_special_tasks',
                             time.time() - self._tick_start)

    @_timer.decorate
    def _schedule_new_jobs(self):
        super(SiteDispatcher, self)._schedule_new_jobs()
        if self._tick_start:
            self._gauge.send('_schedule_new_jobs',
                             time.time() - self._tick_start)


    @_timer.decorate
    def _handle_agents(self):
        super(SiteDispatcher, self)._handle_agents()
        if self._tick_start:
            self._gauge.send('_handle_agents', time.time() - self._tick_start)


    def _reverify_hosts_where(self, where,
                              print_message='Reverifying host %s'):
        """
        This is an altered version of _reverify_hosts_where the class to
        models.SpecialTask.objects.create passes in an argument for
        requested_by, in order to allow the Reset task to be created
        properly.
        """
        full_where='locked = 0 AND invalid = 0 AND ' + where
        for host in scheduler_models.Host.fetch(where=full_where):
            if self.host_has_agent(host):
                # host has already been recovered in some way
                continue
            if self._host_has_scheduled_special_task(host):
                # host will have a special task scheduled on the next cycle
                continue
            if print_message:
                logging.error(print_message, host.hostname)
            try:
                user = models.User.objects.get(login='autotest_system')
            except models.User.DoesNotExist:
                user = models.User.objects.get(
                        id=SiteDispatcher.DEFAULT_REQUESTED_BY_USER_ID)
            models.SpecialTask.objects.create(
                    task=models.SpecialTask.Task.RESET,
                    host=models.Host.objects.get(id=host.id),
                    requested_by=user)


    def _check_for_unrecovered_verifying_entries(self):
        # Verify is replaced by Reset.
        queue_entries = scheduler_models.HostQueueEntry.fetch(
                where='status = "%s"' % models.HostQueueEntry.Status.RESETTING)
        for queue_entry in queue_entries:
            special_tasks = models.SpecialTask.objects.filter(
                    task__in=(models.SpecialTask.Task.CLEANUP,
                              models.SpecialTask.Task.VERIFY,
                              models.SpecialTask.Task.RESET),
                    queue_entry__id=queue_entry.id,
                    is_complete=False)
            if special_tasks.count() == 0:
                logging.error('Unrecovered Resetting host queue entry: %s. '
                              'Setting status to Queued.', str(queue_entry))
                # Essentially this host queue entry was set to be Verifying
                # however no special task exists for entry. This occurs if the
                # scheduler dies between changing the status and creating the
                # special task. By setting it to queued, the job can restart
                # from the beginning and proceed correctly. This is much more
                # preferable than having monitor_db not launching.
                queue_entry.set_status('Queued')
