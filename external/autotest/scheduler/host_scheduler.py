#!/usr/bin/python
#pylint: disable-msg=C0111

# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Host scheduler.

If run as a standalone service, the host scheduler ensures the following:
    1. Hosts will not be assigned to multiple hqes simultaneously. The process
       of assignment in this case refers to the modification of the host_id
       column of a row in the afe_host_queue_entries table, to reflect the host
       id of a leased host matching the dependencies of the job.
    2. Hosts that are not being used by active hqes or incomplete special tasks
       will be released back to the available hosts pool, for acquisition by
       subsequent hqes.
In addition to these guarantees, the host scheduler also confirms that no 2
active hqes/special tasks are assigned the same host, and sets the leased bit
for hosts needed by frontend special tasks. The need for the latter is only
apparent when viewed in the context of the job-scheduler (monitor_db), which
runs special tasks only after their hosts have been leased.

** Suport minimum duts requirement for suites (non-inline mode) **

Each suite can specify the minimum number of duts it requires by
dropping a 'suite_min_duts' job keyval which defaults to 0.

When suites are competing for duts, if any suite has not got minimum duts
it requires, the host scheduler will try to meet the requirement first,
even if other suite may have higher priority or earlier timestamp. Once
all suites' minimum duts requirement have been fullfilled, the host
scheduler will allocate the rest of duts based on job priority and suite job id.
This is to prevent low priority suites from starving when sharing pool with
high-priority suites.

Note:
    1. Prevent potential starvation:
       We need to carefully choose |suite_min_duts| for both low and high
       priority suites. If a high priority suite didn't specify it but a low
       priority one does, the high priority suite can be starved!
    2. Restart requirement:
       Restart host scheduler if you manually released a host by setting
       leased=0 in db. This is needed because host scheduler maintains internal
       state of host assignment for suites.
    3. Exchanging duts triggers provisioning:
       TODO(fdeng): There is a chance two suites can exchange duts,
       if the two suites are for different builds, the exchange
       will trigger provisioning. This can be optimized by preferring getting
       hosts with the same build.
"""

import argparse
import collections
import datetime
import logging
import os
import signal
import sys
import time

import common
from autotest_lib.frontend import setup_django_environment

from autotest_lib.client.common_lib import global_config
from autotest_lib.client.common_lib.cros.graphite import autotest_stats
from autotest_lib.scheduler import email_manager
from autotest_lib.scheduler import query_managers
from autotest_lib.scheduler import rdb_lib
from autotest_lib.scheduler import rdb_utils
from autotest_lib.scheduler import scheduler_lib
from autotest_lib.scheduler import scheduler_models
from autotest_lib.site_utils import job_overhead
from autotest_lib.site_utils import metadata_reporter
from autotest_lib.site_utils import server_manager_utils

_db_manager = None
_shutdown = False
_tick_pause_sec = global_config.global_config.get_config_value(
        'SCHEDULER', 'tick_pause_sec', type=int, default=5)
_monitor_db_host_acquisition = global_config.global_config.get_config_value(
        'SCHEDULER', 'inline_host_acquisition', type=bool, default=True)


class SuiteRecorder(object):
    """Recording the host assignment for suites.

    The recorder holds two things:
        * suite_host_num, records how many duts a suite is holding,
          which is a map <suite_job_id -> num_of_hosts>
        * hosts_to_suites, records which host is assigned to which
          suite, it is a map <host_id -> suite_job_id>
    The two datastructure got updated when a host is assigned to or released
    by a job.

    The reason to maintain hosts_to_suites is that, when a host is released,
    we need to know which suite it was leased to. Querying the db for the
    latest completed job that has run on a host is slow.  Therefore, we go with
    an alternative: keeping a <host id, suite job id> map
    in memory (for 10K hosts, the map should take less than 1M memory on
    64-bit machine with python 2.7)

    """


    _timer = autotest_stats.Timer('suite_recorder')


    def __init__(self, job_query_manager):
        """Initialize.

        @param job_queue_manager: A JobQueueryManager object.
        """
        self.job_query_manager = job_query_manager
        self.suite_host_num, self.hosts_to_suites = (
                self.job_query_manager.get_suite_host_assignment())


    def record_assignment(self, queue_entry):
        """Record that the hqe has got a host.

        @param queue_entry: A scheduler_models.HostQueueEntry object which has
                            got a host.
        """
        parent_id = queue_entry.job.parent_job_id
        if not parent_id:
            return
        if self.hosts_to_suites.get(queue_entry.host_id, None) == parent_id:
            logging.error('HQE (id: %d, parent_job_id: %d, host: %s) '
                          'seems already recorded', queue_entry.id,
                          parent_id, queue_entry.host.hostname)
            return
        num_hosts = self.suite_host_num.get(parent_id, 0)
        self.suite_host_num[parent_id] = num_hosts + 1
        self.hosts_to_suites[queue_entry.host_id] = parent_id
        logging.debug('Suite %d got host %s, currently holding %d hosts',
                      parent_id, queue_entry.host.hostname,
                      self.suite_host_num[parent_id])


    def record_release(self, hosts):
        """Update the record with host releasing event.

        @param hosts: A list of scheduler_models.Host objects.
        """
        for host in hosts:
            if host.id in self.hosts_to_suites:
                parent_job_id = self.hosts_to_suites.pop(host.id)
                count = self.suite_host_num[parent_job_id] - 1
                if count == 0:
                    del self.suite_host_num[parent_job_id]
                else:
                    self.suite_host_num[parent_job_id] = count
                logging.debug(
                        'Suite %d releases host %s, currently holding %d hosts',
                        parent_job_id, host.hostname, count)


    def get_min_duts(self, suite_job_ids):
        """Figure out min duts to request.

        Given a set ids of suite jobs, figure out minimum duts to request for
        each suite. It is determined by two factors: min_duts specified
        for each suite in its job keyvals, and how many duts a suite is
        currently holding.

        @param suite_job_ids: A set of suite job ids.

        @returns: A dictionary, the key is suite_job_id, the value
                  is the minimum number of duts to request.
        """
        suite_min_duts = self.job_query_manager.get_min_duts_of_suites(
                suite_job_ids)
        for parent_id in suite_job_ids:
            min_duts = suite_min_duts.get(parent_id, 0)
            cur_duts = self.suite_host_num.get(parent_id, 0)
            suite_min_duts[parent_id] = max(0, min_duts - cur_duts)
        logging.debug('Minimum duts to get for suites (suite_id: min_duts): %s',
                      suite_min_duts)
        return suite_min_duts


class BaseHostScheduler(object):
    """Base class containing host acquisition logic.

    This class contains all the core host acquisition logic needed by the
    scheduler to run jobs on hosts. It is only capable of releasing hosts
    back to the rdb through its tick, any other action must be instigated by
    the job scheduler.
    """


    _timer = autotest_stats.Timer('base_host_scheduler')
    host_assignment = collections.namedtuple('host_assignment', ['host', 'job'])


    def __init__(self):
        self.host_query_manager = query_managers.AFEHostQueryManager()


    @_timer.decorate
    def _release_hosts(self):
        """Release hosts to the RDB.

        Release all hosts that are ready and are currently not being used by an
        active hqe, and don't have a new special task scheduled against them.

        @return a list of hosts that are released.
        """
        release_hosts = self.host_query_manager.find_unused_healty_hosts()
        release_hostnames = [host.hostname for host in release_hosts]
        if release_hostnames:
            self.host_query_manager.set_leased(
                    False, hostname__in=release_hostnames)
        return release_hosts


    @classmethod
    def schedule_host_job(cls, host, queue_entry):
        """Schedule a job on a host.

        Scheduling a job involves:
            1. Setting the active bit on the queue_entry.
            2. Scheduling a special task on behalf of the queue_entry.
        Performing these actions will lead the job scheduler through a chain of
        events, culminating in running the test and collecting results from
        the host.

        @param host: The host against which to schedule the job.
        @param queue_entry: The queue_entry to schedule.
        """
        if queue_entry.host_id is None:
            queue_entry.set_host(host)
        elif host.id != queue_entry.host_id:
                raise rdb_utils.RDBException('The rdb returned host: %s '
                        'but the job:%s was already assigned a host: %s. ' %
                        (host.hostname, queue_entry.job_id,
                         queue_entry.host.hostname))
        queue_entry.update_field('active', True)

        # TODO: crbug.com/373936. The host scheduler should only be assigning
        # jobs to hosts, but the criterion we use to release hosts depends
        # on it not being used by an active hqe. Since we're activating the
        # hqe here, we also need to schedule its first prejob task. OTOH,
        # we could converge to having the host scheduler manager all special
        # tasks, since their only use today is to verify/cleanup/reset a host.
        logging.info('Scheduling pre job tasks for entry: %s', queue_entry)
        queue_entry.schedule_pre_job_tasks()


    def acquire_hosts(self, host_jobs):
        """Accquire hosts for given jobs.

        This method sends jobs that need hosts to rdb.
        Child class can override this method to pipe more args
        to rdb.

        @param host_jobs: A list of queue entries that either require hosts,
            or require host assignment validation through the rdb.

        @param return: A generator that yields an rdb_hosts.RDBClientHostWrapper
                       for each host acquired on behalf of a queue_entry,
                       or None if a host wasn't found.
        """
        return rdb_lib.acquire_hosts(host_jobs)


    def find_hosts_for_jobs(self, host_jobs):
        """Find and verify hosts for a list of jobs.

        @param host_jobs: A list of queue entries that either require hosts,
            or require host assignment validation through the rdb.
        @return: A list of tuples of the form (host, queue_entry) for each
            valid host-queue_entry assignment.
        """
        jobs_with_hosts = []
        hosts = self.acquire_hosts(host_jobs)
        for host, job in zip(hosts, host_jobs):
            if host:
                jobs_with_hosts.append(self.host_assignment(host, job))
        return jobs_with_hosts


    @_timer.decorate
    def tick(self):
        """Schedule core host management activities."""
        self._release_hosts()


class HostScheduler(BaseHostScheduler):
    """A scheduler capable managing host acquisition for new jobs."""

    _timer = autotest_stats.Timer('host_scheduler')


    def __init__(self):
        super(HostScheduler, self).__init__()
        self.job_query_manager = query_managers.AFEJobQueryManager()
        # Keeping track on how many hosts each suite is holding
        # {suite_job_id: num_hosts}
        self._suite_recorder = SuiteRecorder(self.job_query_manager)


    def _record_host_assignment(self, host, queue_entry):
        """Record that |host| is assigned to |queue_entry|.

        Record:
            1. How long it takes to assign a host to a job in metadata db.
            2. Record host assignment of a suite.

        @param host: A Host object.
        @param queue_entry: A HostQueueEntry object.
        """
        secs_in_queued = (datetime.datetime.now() -
                          queue_entry.job.created_on).total_seconds()
        job_overhead.record_state_duration(
                queue_entry.job_id, host.hostname,
                job_overhead.STATUS.QUEUED, secs_in_queued)
        self._suite_recorder.record_assignment(queue_entry)


    @_timer.decorate
    def _schedule_jobs(self):
        """Schedule new jobs against hosts."""

        key = 'host_scheduler.jobs_per_tick'
        new_jobs_with_hosts = 0
        queue_entries = self.job_query_manager.get_pending_queue_entries(
                only_hostless=False)
        unverified_host_jobs = [job for job in queue_entries
                                if not job.is_hostless()]
        if not unverified_host_jobs:
            return
        for acquisition in self.find_hosts_for_jobs(unverified_host_jobs):
            self.schedule_host_job(acquisition.host, acquisition.job)
            self._record_host_assignment(acquisition.host, acquisition.job)
            new_jobs_with_hosts += 1
        autotest_stats.Gauge(key).send('new_jobs_with_hosts',
                                       new_jobs_with_hosts)
        autotest_stats.Gauge(key).send('new_jobs_without_hosts',
                                       len(unverified_host_jobs) -
                                       new_jobs_with_hosts)


    @_timer.decorate
    def _lease_hosts_of_frontend_tasks(self):
        """Lease hosts of tasks scheduled through the frontend."""
        # We really don't need to get all the special tasks here, just the ones
        # without hqes, but reusing the method used by the scheduler ensures
        # we prioritize the same way.
        lease_hostnames = [
                task.host.hostname for task in
                self.job_query_manager.get_prioritized_special_tasks(
                    only_tasks_with_leased_hosts=False)
                if task.queue_entry_id is None and not task.host.leased]
        # Leasing a leased hosts here shouldn't be a problem:
        # 1. The only way a host can be leased is if it's been assigned to
        #    an active hqe or another similar frontend task, but doing so will
        #    have already precluded it from the list of tasks returned by the
        #    job_query_manager.
        # 2. The unleasing is done based on global conditions. Eg: Even if a
        #    task has already leased a host and we lease it again, the
        #    host scheduler won't release the host till both tasks are complete.
        if lease_hostnames:
            self.host_query_manager.set_leased(
                    True, hostname__in=lease_hostnames)


    def acquire_hosts(self, host_jobs):
        """Override acquire_hosts.

        This method overrides the method in parent class.
        It figures out a set of suites that |host_jobs| belong to;
        and get min_duts requirement for each suite.
        It pipes min_duts for each suite to rdb.

        """
        parent_job_ids = set([q.job.parent_job_id
                              for q in host_jobs if q.job.parent_job_id])
        suite_min_duts = self._suite_recorder.get_min_duts(parent_job_ids)
        return rdb_lib.acquire_hosts(host_jobs, suite_min_duts)


    @_timer.decorate
    def tick(self):
        logging.info('Calling new tick.')
        logging.info('Leasing hosts for frontend tasks.')
        self._lease_hosts_of_frontend_tasks()
        logging.info('Finding hosts for new jobs.')
        self._schedule_jobs()
        logging.info('Releasing unused hosts.')
        released_hosts = self._release_hosts()
        logging.info('Updating suite assignment with released hosts')
        self._suite_recorder.record_release(released_hosts)
        logging.info('Calling email_manager.')
        email_manager.manager.send_queued_emails()


class DummyHostScheduler(BaseHostScheduler):
    """A dummy host scheduler that doesn't acquire or release hosts."""

    def __init__(self):
        pass


    def tick(self):
        pass


def handle_signal(signum, frame):
    """Sigint handler so we don't crash mid-tick."""
    global _shutdown
    _shutdown = True
    logging.info("Shutdown request received.")


def initialize(testing=False):
    """Initialize the host scheduler."""
    if testing:
        # Don't import testing utilities unless we're in testing mode,
        # as the database imports have side effects.
        from autotest_lib.scheduler import rdb_testing_utils
        rdb_testing_utils.FileDatabaseHelper().initialize_database_for_testing(
                db_file_path=rdb_testing_utils.FileDatabaseHelper.DB_FILE)
    global _db_manager
    _db_manager = scheduler_lib.ConnectionManager()
    scheduler_lib.setup_logging(
            os.environ.get('AUTOTEST_SCHEDULER_LOG_DIR', None),
            None, timestamped_logfile_prefix='host_scheduler')
    logging.info("Setting signal handler")
    signal.signal(signal.SIGINT, handle_signal)
    signal.signal(signal.SIGTERM, handle_signal)
    scheduler_models.initialize()


def parse_arguments(argv):
    """
    Parse command line arguments

    @param argv: argument list to parse
    @returns:    parsed arguments.
    """
    parser = argparse.ArgumentParser(description='Host scheduler.')
    parser.add_argument('--testing', action='store_true', default=False,
                        help='Start the host scheduler in testing mode.')
    parser.add_argument('--production',
                        help=('Indicate that scheduler is running in production'
                              ' environment and it can use database that is not'
                              ' hosted in localhost. If it is set to False, '
                              'scheduler will fail if database is not in '
                              'localhost.'),
                        action='store_true', default=False)
    options = parser.parse_args(argv)

    return options


def main():
    if _monitor_db_host_acquisition:
        logging.info('Please set inline_host_acquisition=False in the shadow '
                     'config before starting the host scheduler.')
        # The upstart job for the host scheduler understands exit(0) to mean
        # 'don't respawn'. This is desirable when the job scheduler is acquiring
        # hosts inline.
        sys.exit(0)
    try:
        options = parse_arguments(sys.argv[1:])
        scheduler_lib.check_production_settings(options)

        # If server database is enabled, check if the server has role
        # `host_scheduler`. If the server does not have host_scheduler role,
        # exception will be raised and host scheduler will not continue to run.
        if server_manager_utils.use_server_db():
            server_manager_utils.confirm_server_has_role(hostname='localhost',
                                                         role='host_scheduler')

        initialize(options.testing)

        # Start the thread to report metadata.
        metadata_reporter.start()

        host_scheduler = HostScheduler()
        minimum_tick_sec = global_config.global_config.get_config_value(
                'SCHEDULER', 'minimum_tick_sec', type=float)
        while not _shutdown:
            start = time.time()
            host_scheduler.tick()
            curr_tick_sec = time.time() - start
            if (minimum_tick_sec > curr_tick_sec):
                time.sleep(minimum_tick_sec - curr_tick_sec)
            else:
                time.sleep(0.0001)
    except Exception:
        email_manager.manager.log_stacktrace(
                'Uncaught exception; terminating host_scheduler.')
        raise
    finally:
        email_manager.manager.send_queued_emails()
        if _db_manager:
            _db_manager.disconnect()
        metadata_reporter.abort()


if __name__ == '__main__':
    main()
