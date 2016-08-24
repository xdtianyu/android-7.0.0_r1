#!/usr/bin/python
#
# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import datetime as datetime_base
import logging
from datetime import datetime

import common

from autotest_lib.server import utils
from autotest_lib.server.cros.dynamic_suite import reporting
from autotest_lib.server.cros.dynamic_suite import reporting_utils


class BoardNotAvailableError(utils.TestLabException):
    """Raised when a board is not available in the lab."""


class NotEnoughDutsError(utils.TestLabException):
    """Rasied when the lab doesn't have the minimum number of duts."""


class SimpleTimer(object):
    """A simple timer used to periodically check if a deadline has passed."""

    def _reset(self):
        """Reset the deadline."""
        if not self.interval_hours or self.interval_hours < 0:
            logging.error('Bad interval %s', self.interval_hours)
            self.deadline = None
            return
        self.deadline = datetime.now() + datetime_base.timedelta(
                hours=self.interval_hours)


    def __init__(self, interval_hours=0.5):
        """Initialize a simple periodic deadline timer.

        @param interval_hours: Interval of the deadline.
        """
        self.interval_hours = interval_hours
        self._reset()


    def poll(self):
        """Poll the timer to see if we've hit the deadline.

        This method resets the deadline if it has passed. If the deadline
        hasn't been set, or the current time is less than the deadline, the
        method returns False.

        @return: True if the deadline has passed, False otherwise.
        """
        if not self.deadline or datetime.now() < self.deadline:
            return False
        self._reset()
        return True


class JobTimer(object):
    """Utility class capable of measuring job timeouts.
    """

    # Format used in datetime - string conversion.
    time_format = '%m-%d-%Y [%H:%M:%S]'

    def __init__(self, job_created_time, timeout_mins):
        """JobTimer constructor.

        @param job_created_time: float representing the time a job was
            created. Eg: time.time()
        @param timeout_mins: float representing the timeout in minutes.
        """
        self.job_created_time = datetime.fromtimestamp(job_created_time)
        self.timeout_hours = datetime_base.timedelta(hours=timeout_mins/60.0)
        self.debug_output_timer = SimpleTimer(interval_hours=0.5)
        self.past_halftime = False


    @classmethod
    def format_time(cls, datetime_obj):
        """Get the string formatted version of the datetime object.

        @param datetime_obj: A datetime.datetime object.
            Eg: datetime.datetime.now()

        @return: A formatted string containing the date/time of the
            input datetime.
        """
        return datetime_obj.strftime(cls.time_format)


    def elapsed_time(self):
        """Get the time elapsed since this job was created.

        @return: A timedelta object representing the elapsed time.
        """
        return datetime.now() - self.job_created_time


    def is_suite_timeout(self):
        """Check if the suite timed out.

        @return: True if more than timeout_hours has elapsed since the suite job
            was created.
        """
        if self.elapsed_time() >= self.timeout_hours:
            logging.info('Suite timed out. Started on %s, timed out on %s',
                         self.format_time(self.job_created_time),
                         self.format_time(datetime.now()))
            return True
        return False


    def first_past_halftime(self):
        """Check if we just crossed half time.

        This method will only return True once, the first time it is called
        after a job's elapsed time is past half its timeout.

        @return True: If this is the first call of the method after halftime.
        """
        if (not self.past_halftime and
            self.elapsed_time() > self.timeout_hours/2):
            self.past_halftime = True
            return True
        return False


class RPCHelper(object):
    """A class to help diagnose a suite run through the rpc interface.
    """

    def __init__(self, rpc_interface):
        """Constructor for rpc helper class.

        @param rpc_interface: An rpc object, eg: A RetryingAFE instance.
        """
        self.rpc_interface = rpc_interface
        # Any bug filed during diagnosis. Generally these bugs are critical so
        # there should only be one.
        self.bug = ''


    def diagnose_pool(self, board, pool, time_delta_hours, limit=5):
        """Log diagnostic information about a timeout for a board/pool.

        @param board: The board for which the current suite was run.
        @param pool: The pool against which the current suite was run.
        @param time_delta_hours: The time from which we should log information.
            This is a datetime.timedelta object, as stored by the JobTimer.
        @param limit: The maximum number of jobs per host, to log.

        @raises proxy.JSONRPCException: For exceptions thrown across the wire.
        """
        hosts = self.rpc_interface.get_hosts(
                multiple_labels=('pool:%s' % pool, 'board:%s' % board))
        if not hosts:
            logging.warning('No hosts found for board:%s in pool:%s',
                            board, pool)
            return
        cutoff = datetime.now() - time_delta_hours
        for host in hosts:
            jobs = self.rpc_interface.get_host_queue_entries(
                    host__id=host.id, started_on__gte=str(cutoff))
            job_info = ''
            for job in jobs[-limit:]:
                job_info += ('%s %s started on: %s status %s\n' %
                        (job.id, job.job.name, job.started_on, job.status))
            logging.error('host:%s, status:%s, locked: %s'
                          '\nlabels: %s\nLast %s jobs within %s:\n%s',
                          host.hostname, host.status, host.locked, host.labels,
                          limit, time_delta_hours, job_info)


    def check_dut_availability(self, board, pool, minimum_duts=0):
        """Check if DUT availability for a given board and pool is less than
        minimum.

        @param board: The board to check DUT availability.
        @param pool: The pool to check DUT availability.
        @param minimum_duts: Minimum Number of available machines required to
                             run the suite. Default is set to 0, which means do
                             not force the check of available machines before
                             running the suite.
        @raise: NotEnoughDutsError if DUT availability is lower than minimum.
        @raise: BoardNotAvailableError if no host found for requested
                board/pool.
        """
        if minimum_duts == 0:
            return

        hosts = self.rpc_interface.get_hosts(
                invalid=False,
                multiple_labels=('pool:%s' % pool, 'board:%s' % board))
        if not hosts:
            raise BoardNotAvailableError(
                    'No hosts found for board:%s in pool:%s. The test lab '
                    'currently does not cover test for this board and pool.'%
                    (board, pool))

        if len(hosts) < minimum_duts:
            logging.debug('The total number of DUTs for %s in pool:%s is %d, '
                          'which is no more than the required minimum number of'
                          ' available DUTS of %d. Minimum available DUT rule is'
                          ' not enforced.', board, pool, len(hosts),
                          minimum_duts)
            return

        # TODO(dshi): Replace the hard coded string with enum value,
        # models.Host.Status.REPAIRING and REPAIR_FAILED
        # setup_django_environment can't be imported now as paygen server does
        # not have django package.
        bad_statuses = ('Repair Failed', 'Repairing', 'Verifying')
        unusable_hosts = []
        available_hosts = []
        for host in hosts:
            if host.status in bad_statuses or host.locked:
                unusable_hosts.append(host.hostname)
            else:
                available_hosts.append(host)
        logging.debug('%d of %d DUTs are available for board %s pool %s.',
                      len(available_hosts), len(hosts), board, pool)
        if len(available_hosts) < minimum_duts:
            if unusable_hosts:
                pool_health_bug = reporting.PoolHealthBug(
                        pool, board, unusable_hosts)
                self.bug = reporting.Reporter().report(pool_health_bug)[0]
            raise NotEnoughDutsError(
                    'Number of available DUTs for board %s pool %s is %d, which'
                    ' is less than the minimum value %d. '
                    'Filed https://crbug.com/%s' %
                    (board, pool, len(available_hosts), minimum_duts, self.bug))


    def diagnose_job(self, job_id, instance_server):
        """Diagnose a suite job.

        Logs information about the jobs that are still to run in the suite.

        @param job_id: The id of the suite job to get information about.
            No meaningful information gets logged if the id is for a sub-job.
        @param instance_server: The instance server.
            Eg: cautotest, cautotest-cq, localhost.
        """
        incomplete_jobs = self.rpc_interface.get_jobs(
                parent_job_id=job_id, summary=True,
                hostqueueentry__complete=False)
        if incomplete_jobs:
            logging.info('\n%s printing summary of incomplete jobs (%s):\n',
                         JobTimer.format_time(datetime.now()),
                         len(incomplete_jobs))
            for job in incomplete_jobs:
                logging.info('%s: %s', job.testname[job.testname.rfind('/')+1:],
                             reporting_utils.link_job(job.id, instance_server))
        else:
            logging.info('All jobs in suite have already completed.')
