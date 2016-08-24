# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import datetime
import logging

import common
from autotest_lib.client.common_lib import error
from autotest_lib.server import site_utils
from autotest_lib.server.cros import provision
from autotest_lib.server.cros.dynamic_suite import frontend_wrappers, reporting


# Number of days back to search for existing job.
SEARCH_JOB_MAX_DAYS = 14

class DedupingSchedulerException(Exception):
    """Base class for exceptions from this module."""
    pass


class ScheduleException(DedupingSchedulerException):
    """Raised when an error is returned from the AFE during scheduling."""
    pass


class DedupException(DedupingSchedulerException):
    """Raised when an error occurs while checking for duplicate jobs."""
    pass


class DedupingScheduler(object):
    """A class that will schedule suites to run on a given board, build.

    Includes logic to check whether or not a given (suite, board, build)
    has already been run.  If so, it will skip scheduling that suite.

    @var _afe: a frontend.AFE instance used to talk to autotest.
    """


    def __init__(self, afe=None, file_bug=False):
        """Constructor

        @param afe: an instance of AFE as defined in server/frontend.py.
                    Defaults to a frontend_wrappers.RetryingAFE instance.
        """
        self._afe = afe or frontend_wrappers.RetryingAFE(timeout_min=30,
                                                         delay_sec=10,
                                                         debug=False)
        self._file_bug = file_bug


    def _ShouldScheduleSuite(self, suite, board, test_source_build):
        """Return True if |suite| has not yet been run for |build| on |board|.

        True if |suite| has not been run for |build| on |board|, and
        the lab is open for this particular request.  False otherwise.

        @param suite: the name of the suite to run, e.g. 'bvt'
        @param board: the board to run the suite on, e.g. x86-alex
        @param test_source_build: Build with the source of tests.

        @return False if the suite was already scheduled, True if not
        @raise DedupException if the AFE raises while searching for jobs.

        """
        try:
            site_utils.check_lab_status(test_source_build)
        except site_utils.TestLabException as ex:
            logging.debug('Skipping suite %s, board %s, build %s:  %s',
                          suite, board, test_source_build, str(ex))
            return False
        try:
            start_time = str(datetime.datetime.now() -
                             datetime.timedelta(days=SEARCH_JOB_MAX_DAYS))
            return not self._afe.get_jobs(
                    name__startswith=test_source_build,
                    name__endswith='control.'+suite,
                    created_on__gte=start_time)
        except Exception as e:
            raise DedupException(e)


    def _Schedule(self, suite, board, build, pool, num, priority, timeout,
                  file_bugs=False, firmware_rw_build=None,
                  test_source_build=None, job_retry=False):
        """Schedule |suite|, if it hasn't already been run.

        @param suite: the name of the suite to run, e.g. 'bvt'
        @param board: the board to run the suite on, e.g. x86-alex
        @param build: the build to install e.g.
                      x86-alex-release/R18-1655.0.0-a1-b1584.
        @param pool: the pool of machines to use for scheduling purposes.
                     Default: None
        @param num: the number of devices across which to shard the test suite.
                    Type: integer or None
                    Default: None (uses sharding factor in global_config.ini).
        @param priority: One of the values from
                         client.common_lib.priorities.Priority.
        @param timeout: The max lifetime of the suite in hours.
        @param file_bugs: True if bug filing is desired for this suite.
        @param firmware_rw_build: Firmware build to update RW firmware. Default
                                  to None.
        @param test_source_build: Build that contains the server-side test code.
                                  Default to None to use the ChromeOS build
                                  (defined by `build`).
        @param job_retry: Set to True to enable job-level retry. Default is
                          False.

        @return True if the suite got scheduled
        @raise ScheduleException if an error occurs while scheduling.

        """
        try:
            builds = {provision.CROS_VERSION_PREFIX: build}
            if firmware_rw_build:
                builds[provision.FW_RW_VERSION_PREFIX] = firmware_rw_build
            logging.info('Scheduling %s on %s against %s (pool: %s)',
                         suite, builds, board, pool)
            if self._afe.run(
                        'create_suite_job', name=suite, board=board,
                        builds=builds, check_hosts=False, num=num, pool=pool,
                        priority=priority, timeout=timeout, file_bugs=file_bugs,
                        wait_for_results=file_bugs,
                        test_source_build=test_source_build,
                        job_retry=job_retry) is not None:
                return True
            else:
                raise ScheduleException(
                    "Can't schedule %s for %s." % (suite, builds))
        except (error.ControlFileNotFound, error.ControlFileEmpty,
                error.ControlFileMalformed, error.NoControlFileList) as e:
            if self._file_bug:
                # File bug on test_source_build if it's specified.
                b = reporting.SuiteSchedulerBug(
                        suite, test_source_build or build, board, e)
                # If a bug has filed with the same <suite, build, error type>
                # will not file again, but simply gets the existing bug id.
                bid, _ = reporting.Reporter().report(
                        b, ignore_duplicate=True)
                if bid is not None:
                    return False
            # Raise the exception if not filing a bug or failed to file bug.
            raise ScheduleException(e)
        except Exception as e:
            raise ScheduleException(e)


    def ScheduleSuite(self, suite, board, build, pool, num, priority, timeout,
                      force=False, file_bugs=False, firmware_rw_build=None,
                      test_source_build=None, job_retry=False):
        """Schedule |suite|, if it hasn't already been run.

        If |suite| has not already been run against |build| on |board|,
        schedule it and return True.  If it has, return False.

        @param suite: the name of the suite to run, e.g. 'bvt'
        @param board: the board to run the suite on, e.g. x86-alex
        @param build: the ChromeOS build to install e.g.
                      x86-alex-release/R18-1655.0.0-a1-b1584.
        @param pool: the pool of machines to use for scheduling purposes.
        @param num: the number of devices across which to shard the test suite.
                    Type: integer or None
        @param priority: One of the values from
                         client.common_lib.priorities.Priority.
        @param timeout: The max lifetime of the suite in hours.
        @param force: Always schedule the suite.
        @param file_bugs: True if bug filing is desired for this suite.
        @param firmware_rw_build: Firmware build to update RW firmware. Default
                                  to None.
        @param test_source_build: Build with the source of tests. Default to
                                  None to use the ChromeOS build.
        @param job_retry: Set to True to enable job-level retry. Default is
                          False.

        @return True if the suite got scheduled, False if not
        @raise DedupException if we can't check for dups.
        @raise ScheduleException if the suite cannot be scheduled.

        """
        if (force or self._ShouldScheduleSuite(suite, board,
                                               test_source_build or build)):
            return self._Schedule(suite, board, build, pool, num, priority,
                                  timeout, file_bugs=file_bugs,
                                  firmware_rw_build=firmware_rw_build,
                                  test_source_build=test_source_build,
                                  job_retry=job_retry)
        return False


    def CheckHostsExist(self, *args, **kwargs):
        """Forward a request to check if hosts matching args, kwargs exist."""
        try:
            return self._afe.get_hostnames(*args, **kwargs)
        except error.TimeoutException as e:
            logging.exception(e)
            return []
