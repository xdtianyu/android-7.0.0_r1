# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import datetime
import difflib
import hashlib
import logging
import operator
import os
import re
import traceback
import sys

import common

from autotest_lib.frontend.afe.json_rpc import proxy
from autotest_lib.client.common_lib import control_data
from autotest_lib.client.common_lib import enum
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import global_config
from autotest_lib.client.common_lib import priorities
from autotest_lib.client.common_lib import site_utils
from autotest_lib.client.common_lib import time_utils
from autotest_lib.client.common_lib import utils
from autotest_lib.frontend.afe.json_rpc import proxy
from autotest_lib.server.cros import provision
from autotest_lib.server.cros.dynamic_suite import constants
from autotest_lib.server.cros.dynamic_suite import control_file_getter
from autotest_lib.server.cros.dynamic_suite import frontend_wrappers
from autotest_lib.server.cros.dynamic_suite import job_status
from autotest_lib.server.cros.dynamic_suite import reporting
from autotest_lib.server.cros.dynamic_suite import reporting_utils
from autotest_lib.server.cros.dynamic_suite import tools
from autotest_lib.server.cros.dynamic_suite.job_status import Status

try:
    from chromite.lib import boolparse_lib
    from chromite.lib import cros_logging as logging
except ImportError:
    print 'Unable to import chromite.'
    print 'This script must be either:'
    print '  - Be run in the chroot.'
    print '  - (not yet supported) be run after running '
    print '    ../utils/build_externals.py'

_FILE_BUG_SUITES = ['au', 'bvt', 'bvt-cq', 'bvt-inline', 'paygen_au_beta',
                    'paygen_au_canary', 'paygen_au_dev', 'paygen_au_stable',
                    'sanity', 'push_to_prod']
_AUTOTEST_DIR = global_config.global_config.get_config_value(
        'SCHEDULER', 'drone_installation_directory')


class RetryHandler(object):
    """Maintain retry information.

    @var _retry_map: A dictionary that stores retry history.
            The key is afe job id. The value is a dictionary.
            {job_id: {'state':RetryHandler.States, 'retry_max':int}}
            - state:
                The retry state of a job.
                NOT_ATTEMPTED:
                    We haven't done anything about the job.
                ATTEMPTED:
                    We've made an attempt to schedule a retry job. The
                    scheduling may or may not be successful, e.g.
                    it might encounter an rpc error. Note failure
                    in scheduling a retry is different from a retry job failure.
                    For each job, we only attempt to schedule a retry once.
                    For example, assume we have a test with JOB_RETRIES=5 and
                    its second retry job failed. When we attempt to create
                    a third retry job to retry the second, we hit an rpc
                    error. In such case, we will give up on all following
                    retries.
                RETRIED:
                    A retry job has already been successfully
                    scheduled.
            - retry_max:
                The maximum of times the job can still
                be retried, taking into account retries
                that have occurred.
    @var _retry_level: A retry might be triggered only if the result
            is worse than the level.
    @var _max_retries: Maximum retry limit at suite level.
                     Regardless how many times each individual test
                     has been retried, the total number of retries happening in
                     the suite can't exceed _max_retries.
    """

    States = enum.Enum('NOT_ATTEMPTED', 'ATTEMPTED', 'RETRIED',
                       start_value=1, step=1)

    def __init__(self, initial_jobs_to_tests, retry_level='WARN',
                 max_retries=None):
        """Initialize RetryHandler.

        @param initial_jobs_to_tests: A dictionary that maps a job id to
                a ControlData object. This dictionary should contain
                jobs that are originally scheduled by the suite.
        @param retry_level: A retry might be triggered only if the result is
                worse than the level.
        @param max_retries: Integer, maxmium total retries allowed
                                  for the suite. Default to None, no max.
        """
        self._retry_map = {}
        self._retry_level = retry_level
        self._max_retries = (max_retries
                             if max_retries is not None else sys.maxint)
        for job_id, test in initial_jobs_to_tests.items():
            if test.job_retries > 0:
                self.add_job(new_job_id=job_id,
                             retry_max=test.job_retries)


    def add_job(self, new_job_id, retry_max):
        """Add a newly-created job to the retry map.

        @param new_job_id: The afe_job_id of a newly created job.
        @param retry_max: The maximum of times that we could retry
                          the test if the job fails.

        @raises ValueError if new_job_id is already in retry map.

        """
        if new_job_id in self._retry_map:
            raise ValueError('add_job called when job is already in retry map.')

        self._retry_map[new_job_id] = {
                'state': self.States.NOT_ATTEMPTED,
                'retry_max': retry_max}


    def suite_max_reached(self):
        """Return whether maximum retry limit for a suite has been reached."""
        return self._max_retries <= 0


    def should_retry(self, result):
        """Check whether we should retry a job based on its result.

        We will retry the job that corresponds to the result
        when all of the following are true.
        a) The test was actually executed, meaning that if
           a job was aborted before it could ever reach the state
           of 'Running', the job will not be retried.
        b) The result is worse than |self._retry_level| which
           defaults to 'WARN'.
        c) The test requires retry, i.e. the job has an entry in the retry map.
        d) We haven't made any retry attempt yet, i.e. state == NOT_ATTEMPTED
           Note that if a test has JOB_RETRIES=5, and the second time
           it was retried it hit an rpc error, we will give up on
           all following retries.
        e) The job has not reached its retry max, i.e. retry_max > 0

        @param result: A result, encapsulating the status of the job.

        @returns: True if we should retry the job.

        """
        if (self.suite_max_reached() or not result.test_executed or
            not result.is_worse_than(
                job_status.Status(self._retry_level, '', 'reason'))):
            return False
        failed_job_id = result.id
        return (failed_job_id in self._retry_map and
                self._retry_map[failed_job_id]['state'] ==
                        self.States.NOT_ATTEMPTED and
                self._retry_map[failed_job_id]['retry_max'] > 0)


    def add_retry(self, old_job_id, new_job_id):
        """Record a retry.

        Update retry map with the retry information.

        @param old_job_id: The afe_job_id of the job that is retried.
        @param new_job_id: The afe_job_id of the retry job.

        @raises KeyError if old_job_id isn't in the retry map.
        @raises ValueError if we have already retried or made an attempt
                to retry the old job.

        """
        old_record = self._retry_map[old_job_id]
        if old_record['state'] != self.States.NOT_ATTEMPTED:
            raise ValueError(
                    'We have already retried or attempted to retry job %d' %
                    old_job_id)
        old_record['state'] = self.States.RETRIED
        self.add_job(new_job_id=new_job_id,
                     retry_max=old_record['retry_max'] - 1)
        self._max_retries -= 1


    def set_attempted(self, job_id):
        """Set the state of the job to ATTEMPTED.

        @param job_id: afe_job_id of a job.

        @raises KeyError if job_id isn't in the retry map.
        @raises ValueError if the current state is not NOT_ATTEMPTED.

        """
        current_state = self._retry_map[job_id]['state']
        if current_state != self.States.NOT_ATTEMPTED:
            # We are supposed to retry or attempt to retry each job
            # only once. Raise an error if this is not the case.
            raise ValueError('Unexpected state transition: %s -> %s' %
                             (self.States.get_string(current_state),
                              self.States.get_string(self.States.ATTEMPTED)))
        else:
            self._retry_map[job_id]['state'] = self.States.ATTEMPTED


    def has_following_retry(self, result):
        """Check whether there will be a following retry.

        We have the following cases for a given job id (result.id),
        - no retry map entry -> retry not required, no following retry
        - has retry map entry:
            - already retried -> has following retry
            - has not retried
                (this branch can be handled by checking should_retry(result))
                - retry_max == 0 --> the last retry job, no more retry
                - retry_max > 0
                   - attempted, but has failed in scheduling a
                     following retry due to rpc error  --> no more retry
                   - has not attempped --> has following retry if test failed.

        @param result: A result, encapsulating the status of the job.

        @returns: True, if there will be a following retry.
                  False otherwise.

        """
        return (result.test_executed and result.id in self._retry_map and (
                self._retry_map[result.id]['state'] == self.States.RETRIED or
                self.should_retry(result)))


    def get_retry_max(self, job_id):
        """Get the maximum times the job can still be retried.

        @param job_id: afe_job_id of a job.

        @returns: An int, representing the maximum times the job can still be
                  retried.
        @raises KeyError if job_id isn't in the retry map.

        """
        return self._retry_map[job_id]['retry_max']


class Suite(object):
    """
    A suite of tests, defined by some predicate over control file variables.

    Given a place to search for control files a predicate to match the desired
    tests, can gather tests and fire off jobs to run them, and then wait for
    results.

    @var _predicate: a function that should return True when run over a
         ControlData representation of a control file that should be in
         this Suite.
    @var _tag: a string with which to tag jobs run in this suite.
    @var _builds: the builds on which we're running this suite.
    @var _afe: an instance of AFE as defined in server/frontend.py.
    @var _tko: an instance of TKO as defined in server/frontend.py.
    @var _jobs: currently scheduled jobs, if any.
    @var _jobs_to_tests: a dictionary that maps job ids to tests represented
                         ControlData objects.
    @var _cf_getter: a control_file_getter.ControlFileGetter
    @var _retry: a bool value indicating whether jobs should be retried on
                 failure.
    @var _retry_handler: a RetryHandler object.

    """


    @staticmethod
    def create_ds_getter(build, devserver):
        """
        @param build: the build on which we're running this suite.
        @param devserver: the devserver which contains the build.
        @return a FileSystemGetter instance that looks under |autotest_dir|.
        """
        return control_file_getter.DevServerGetter(build, devserver)


    @staticmethod
    def create_fs_getter(autotest_dir):
        """
        @param autotest_dir: the place to find autotests.
        @return a FileSystemGetter instance that looks under |autotest_dir|.
        """
        # currently hard-coded places to look for tests.
        subpaths = ['server/site_tests', 'client/site_tests',
                    'server/tests', 'client/tests']
        directories = [os.path.join(autotest_dir, p) for p in subpaths]
        return control_file_getter.FileSystemGetter(directories)


    @staticmethod
    def parse_tag(tag):
        """Splits a string on ',' optionally surrounded by whitespace.
        @param tag: string to split.
        """
        return map(lambda x: x.strip(), tag.split(','))


    @staticmethod
    def name_in_tag_predicate(name):
        """Returns predicate that takes a control file and looks for |name|.

        Builds a predicate that takes in a parsed control file (a ControlData)
        and returns True if the SUITE tag is present and contains |name|.

        @param name: the suite name to base the predicate on.
        @return a callable that takes a ControlData and looks for |name| in that
                ControlData object's suite member.
        """
        return lambda t: (hasattr(t, 'suite') and
                          name in Suite.parse_tag(t.suite))


    @staticmethod
    def name_in_tag_similarity_predicate(name):
        """Returns predicate that takes a control file and gets the similarity
        of the suites in the control file and the given name.

        Builds a predicate that takes in a parsed control file (a ControlData)
        and returns a list of tuples of (suite name, ratio), where suite name
        is each suite listed in the control file, and ratio is the similarity
        between each suite and the given name.

        @param name: the suite name to base the predicate on.
        @return a callable that takes a ControlData and returns a list of tuples
                of (suite name, ratio), where suite name is each suite listed in
                the control file, and ratio is the similarity between each suite
                and the given name.
        """
        return lambda t: ((None, 0) if not hasattr(t, 'suite') else
                          [(suite,
                            difflib.SequenceMatcher(a=suite, b=name).ratio())
                           for suite in Suite.parse_tag(t.suite)])


    @staticmethod
    def not_in_blacklist_predicate(blacklist):
        """Returns predicate that takes a control file and looks for its
        path to not be in given blacklist.

        @param blacklist: A list of strings both paths on control_files that
                          should be blacklisted.

        @return a callable that takes a ControlData and looks for it to be
                absent from blacklist.
        """
        return lambda t: hasattr(t, 'path') and \
                         not any(b.endswith(t.path) for b in blacklist)


    @staticmethod
    def test_name_equals_predicate(test_name):
        """Returns predicate that matched based on a test's name.

        Builds a predicate that takes in a parsed control file (a ControlData)
        and returns True if the test name is equal to |test_name|.

        @param test_name: the test name to base the predicate on.
        @return a callable that takes a ControlData and looks for |test_name|
                in that ControlData's name.
        """
        return lambda t: hasattr(t, 'name') and test_name == t.name


    @staticmethod
    def test_name_matches_pattern_predicate(test_name_pattern):
        """Returns predicate that matches based on a test's name pattern.

        Builds a predicate that takes in a parsed control file (a ControlData)
        and returns True if the test name matches the given regular expression.

        @param test_name_pattern: regular expression (string) to match against
                                  test names.
        @return a callable that takes a ControlData and returns
                True if the name fields matches the pattern.
        """
        return lambda t: hasattr(t, 'name') and re.match(test_name_pattern,
                                                         t.name)


    @staticmethod
    def test_file_matches_pattern_predicate(test_file_pattern):
        """Returns predicate that matches based on a test's file name pattern.

        Builds a predicate that takes in a parsed control file (a ControlData)
        and returns True if the test's control file name matches the given
        regular expression.

        @param test_file_pattern: regular expression (string) to match against
                                  control file names.
        @return a callable that takes a ControlData and and returns
                True if control file name matches the pattern.
        """
        return lambda t: hasattr(t, 'path') and re.match(test_file_pattern,
                                                         t.path)


    @staticmethod
    def matches_attribute_expression_predicate(test_attr_boolstr):
        """Returns predicate that matches based on boolean expression of
        attributes.

        Builds a predicate that takes in a parsed control file (a ControlData)
        ans returns True if the test attributes satisfy the given attribute
        boolean expression.

        @param test_attr_boolstr: boolean expression of the attributes to be
                                  test, like 'system:all and interval:daily'.

        @return a callable that takes a ControlData and returns True if the test
                attributes satisfy the given boolean expression.
        """
        return lambda t: boolparse_lib.BoolstrResult(
            test_attr_boolstr, t.attributes)

    @staticmethod
    def test_name_similarity_predicate(test_name):
        """Returns predicate that matched based on a test's name.

        Builds a predicate that takes in a parsed control file (a ControlData)
        and returns a tuple of (test name, ratio), where ratio is the similarity
        between the test name and the given test_name.

        @param test_name: the test name to base the predicate on.
        @return a callable that takes a ControlData and returns a tuple of
                (test name, ratio), where ratio is the similarity between the
                test name and the given test_name.
        """
        return lambda t: ((None, 0) if not hasattr(t, 'name') else
                (t.name,
                 difflib.SequenceMatcher(a=t.name, b=test_name).ratio()))


    @staticmethod
    def test_file_similarity_predicate(test_file_pattern):
        """Returns predicate that gets the similarity based on a test's file
        name pattern.

        Builds a predicate that takes in a parsed control file (a ControlData)
        and returns a tuple of (file path, ratio), where ratio is the
        similarity between the test file name and the given test_file_pattern.

        @param test_file_pattern: regular expression (string) to match against
                                  control file names.
        @return a callable that takes a ControlData and and returns a tuple of
                (file path, ratio), where ratio is the similarity between the
                test file name and the given test_file_pattern.
        """
        return lambda t: ((None, 0) if not hasattr(t, 'path') else
                (t.path, difflib.SequenceMatcher(a=t.path,
                                                 b=test_file_pattern).ratio()))


    @staticmethod
    def list_all_suites(build, devserver, cf_getter=None):
        """
        Parses all ControlData objects with a SUITE tag and extracts all
        defined suite names.

        @param build: the build on which we're running this suite.
        @param devserver: the devserver which contains the build.
        @param cf_getter: control_file_getter.ControlFileGetter. Defaults to
                          using DevServerGetter.

        @return list of suites
        """
        if cf_getter is None:
            cf_getter = Suite.create_ds_getter(build, devserver)

        suites = set()
        predicate = lambda t: hasattr(t, 'suite')
        for test in Suite.find_and_parse_tests(cf_getter, predicate,
                                               add_experimental=True):
            suites.update(Suite.parse_tag(test.suite))
        return list(suites)


    @staticmethod
    def get_test_source_build(builds, **dargs):
        """Get the build of test code.

        Get the test source build from arguments. If parameter
        `test_source_build` is set and has a value, return its value. Otherwise
        returns the ChromeOS build name if it exists. If ChromeOS build is not
        specified either, raise SuiteArgumentException.

        @param builds: the builds on which we're running this suite. It's a
                       dictionary of version_prefix:build.
        @param **dargs: Any other Suite constructor parameters, as described
                        in Suite.__init__ docstring.

        @return: The build contains the test code.
        @raise: SuiteArgumentException if both test_source_build and ChromeOS
                build are not specified.

        """
        if dargs.get('test_source_build', None):
            return dargs['test_source_build']
        test_source_build = builds.get(provision.CROS_VERSION_PREFIX, None)
        if not test_source_build:
            raise error.SuiteArgumentException(
                    'test_source_build must be specified if CrOS build is not '
                    'specified.')
        return test_source_build


    @staticmethod
    def create_from_predicates(predicates, builds, board, devserver,
                               cf_getter=None, name='ad_hoc_suite',
                               run_prod_code=False, **dargs):
        """
        Create a Suite using a given predicate test filters.

        Uses supplied predicate(s) to instantiate a Suite. Looks for tests in
        |autotest_dir| and will schedule them using |afe|.  Pulls control files
        from the default dev server. Results will be pulled from |tko| upon
        completion.

        @param predicates: A list of callables that accept ControlData
                           representations of control files. A test will be
                           included in suite if all callables in this list
                           return True on the given control file.
        @param builds: the builds on which we're running this suite. It's a
                       dictionary of version_prefix:build.
        @param board: the board on which we're running this suite.
        @param devserver: the devserver which contains the build.
        @param cf_getter: control_file_getter.ControlFileGetter. Defaults to
                          using DevServerGetter.
        @param name: name of suite. Defaults to 'ad_hoc_suite'
        @param run_prod_code: If true, the suite will run the tests that
                              lives in prod aka the test code currently on the
                              lab servers.
        @param **dargs: Any other Suite constructor parameters, as described
                        in Suite.__init__ docstring.
        @return a Suite instance.
        """
        if cf_getter is None:
            build = Suite.get_test_source_build(builds, **dargs)
            if run_prod_code:
                cf_getter = Suite.create_fs_getter(_AUTOTEST_DIR)
            else:
                cf_getter = Suite.create_ds_getter(build, devserver)

        return Suite(predicates,
                     name, builds, board, cf_getter, run_prod_code, **dargs)


    @staticmethod
    def create_from_name(name, builds, board, devserver, cf_getter=None,
                         **dargs):
        """
        Create a Suite using a predicate based on the SUITE control file var.

        Makes a predicate based on |name| and uses it to instantiate a Suite
        that looks for tests in |autotest_dir| and will schedule them using
        |afe|.  Pulls control files from the default dev server.
        Results will be pulled from |tko| upon completion.

        @param name: a value of the SUITE control file variable to search for.
        @param builds: the builds on which we're running this suite. It's a
                       dictionary of version_prefix:build.
        @param board: the board on which we're running this suite.
        @param devserver: the devserver which contains the build.
        @param cf_getter: control_file_getter.ControlFileGetter. Defaults to
                          using DevServerGetter.
        @param **dargs: Any other Suite constructor parameters, as described
                        in Suite.__init__ docstring.
        @return a Suite instance.
        """
        if cf_getter is None:
            build = Suite.get_test_source_build(builds, **dargs)
            cf_getter = Suite.create_ds_getter(build, devserver)

        return Suite([Suite.name_in_tag_predicate(name)],
                     name, builds, board, cf_getter, **dargs)


    def __init__(self, predicates, tag, builds, board, cf_getter,
                 run_prod_code=False, afe=None, tko=None, pool=None,
                 results_dir=None, max_runtime_mins=24*60, timeout_mins=24*60,
                 file_bugs=False, file_experimental_bugs=False,
                 suite_job_id=None, ignore_deps=False, extra_deps=[],
                 priority=priorities.Priority.DEFAULT, forgiving_parser=True,
                 wait_for_results=True, job_retry=False,
                 max_retries=sys.maxint, offload_failures_only=False,
                 test_source_build=None):
        """
        Constructor

        @param predicates: A list of callables that accept ControlData
                           representations of control files. A test will be
                           included in suite is all callables in this list
                           return True on the given control file.
        @param tag: a string with which to tag jobs run in this suite.
        @param builds: the builds on which we're running this suite.
        @param board: the board on which we're running this suite.
        @param cf_getter: a control_file_getter.ControlFileGetter
        @param afe: an instance of AFE as defined in server/frontend.py.
        @param tko: an instance of TKO as defined in server/frontend.py.
        @param pool: Specify the pool of machines to use for scheduling
                purposes.
        @param run_prod_code: If true, the suite will run the test code that
                              lives in prod aka the test code currently on the
                              lab servers.
        @param results_dir: The directory where the job can write results to.
                            This must be set if you want job_id of sub-jobs
                            list in the job keyvals.
        @param max_runtime_mins: Maximum suite runtime, in minutes.
        @param timeout: Maximum job lifetime, in hours.
        @param suite_job_id: Job id that will act as parent id to all sub jobs.
                             Default: None
        @param ignore_deps: True if jobs should ignore the DEPENDENCIES
                            attribute and skip applying of dependency labels.
                            (Default:False)
        @param extra_deps: A list of strings which are the extra DEPENDENCIES
                           to add to each test being scheduled.
        @param priority: Integer priority level.  Higher is more important.
        @param wait_for_results: Set to False to run the suite job without
                                 waiting for test jobs to finish. Default is
                                 True.
        @param job_retry: A bool value indicating whether jobs should be retired
                          on failure. If True, the field 'JOB_RETRIES' in
                          control files will be respected. If False, do not
                          retry.
        @param max_retries: Maximum retry limit at suite level.
                            Regardless how many times each individual test
                            has been retried, the total number of retries
                            happening in the suite can't exceed _max_retries.
                            Default to sys.maxint.
        @param offload_failures_only: Only enable gs_offloading for failed
                                      jobs.
        @param test_source_build: Build that contains the server-side test code.

        """
        def combined_predicate(test):
            #pylint: disable-msg=C0111
            return all((f(test) for f in predicates))
        self._predicate = combined_predicate

        self._tag = tag
        self._builds = builds
        self._board = board
        self._cf_getter = cf_getter
        self._results_dir = results_dir
        self._afe = afe or frontend_wrappers.RetryingAFE(timeout_min=30,
                                                         delay_sec=10,
                                                         debug=False)
        self._tko = tko or frontend_wrappers.RetryingTKO(timeout_min=30,
                                                         delay_sec=10,
                                                         debug=False)
        self._pool = pool
        self._jobs = []
        self._jobs_to_tests = {}
        self._tests = Suite.find_and_parse_tests(self._cf_getter,
                        self._predicate, self._tag, add_experimental=True,
                        forgiving_parser=forgiving_parser,
                        run_prod_code=run_prod_code)

        self._max_runtime_mins = max_runtime_mins
        self._timeout_mins = timeout_mins
        self._file_bugs = file_bugs
        self._file_experimental_bugs = file_experimental_bugs
        self._suite_job_id = suite_job_id
        self._ignore_deps = ignore_deps
        self._extra_deps = extra_deps
        self._priority = priority
        self._job_retry=job_retry
        self._max_retries = max_retries
        # RetryHandler to be initialized in schedule()
        self._retry_handler = None
        self.wait_for_results = wait_for_results
        self._offload_failures_only = offload_failures_only
        self._test_source_build = test_source_build


    @property
    def tests(self):
        """
        A list of ControlData objects in the suite, with added |text| attr.
        """
        return self._tests


    def stable_tests(self):
        """
        |self.tests|, filtered for non-experimental tests.
        """
        return filter(lambda t: not t.experimental, self.tests)


    def unstable_tests(self):
        """
        |self.tests|, filtered for experimental tests.
        """
        return filter(lambda t: t.experimental, self.tests)


    def _create_job(self, test, retry_for=None):
        """
        Thin wrapper around frontend.AFE.create_job().

        @param test: ControlData object for a test to run.
        @param retry_for: If the to-be-created job is a retry for an
                          old job, the afe_job_id of the old job will
                          be passed in as |retry_for|, which will be
                          recorded in the new job's keyvals.
        @returns: A frontend.Job object with an added test_name member.
                  test_name is used to preserve the higher level TEST_NAME
                  name of the job.
        """
        if self._ignore_deps:
            job_deps = []
        else:
            job_deps = list(test.dependencies)
        if self._extra_deps:
            job_deps.extend(self._extra_deps)
        if self._pool:
            job_deps.append(self._pool)

        # TODO(beeps): Comletely remove the concept of a metahost.
        # Currently we use this to distinguis a job scheduled through
        # the afe from a suite job, as only the latter will get requeued
        # when a special task fails.
        job_deps.append(self._board)
        # JOB_BUILD_KEY is default to use CrOS image, if it's not available,
        # take the first build in the builds dictionary.
        # test_source_build is saved to job_keyvals so scheduler can retrieve
        # the build name from database when compiling autoserv commandline.
        # This avoid a database change to add a new field in afe_jobs.
        build = self._builds.get(provision.CROS_VERSION_PREFIX,
                                 self._builds.values()[0])
        keyvals={constants.JOB_BUILD_KEY: build,
                 constants.JOB_SUITE_KEY: self._tag,
                 constants.JOB_EXPERIMENTAL_KEY: test.experimental,
                 constants.JOB_BUILDS_KEY: self._builds}
        # Only add `test_source_build` to job keyvals if the build is different
        # from the CrOS build or the job uses more than one build, e.g., both
        # firmware and CrOS will be updated in the dut.
        # This is for backwards compatibility, so the update Autotest code can
        # compile an autoserv command line to run in a SSP container using
        # previous builds.
        if (self._test_source_build and
            (build != self._test_source_build or len(self._builds) > 1)):
            keyvals[constants.JOB_TEST_SOURCE_BUILD_KEY] = (
                    self._test_source_build)
            for prefix, build in self._builds.iteritems():
                if prefix == provision.FW_RW_VERSION_PREFIX:
                    keyvals[constants.FWRW_BUILD]= build
                elif prefix == provision.FW_RO_VERSION_PREFIX:
                    keyvals[constants.FWRO_BUILD] = build
        # Add suite job id to keyvals so tko parser can read it from keyval file
        if self._suite_job_id:
            keyvals[constants.PARENT_JOB_ID] = self._suite_job_id
        if retry_for:
            # We drop the old job's id in the new job's keyval file
            # so that later our tko parser can figure out the retring
            # relationship and invalidate the results of the old job
            # in tko database.
            keyvals[constants.RETRY_ORIGINAL_JOB_ID] = retry_for
        if self._offload_failures_only:
            keyvals[constants.JOB_OFFLOAD_FAILURES_KEY] = True

        test_obj = self._afe.create_job(
            control_file=test.text,
            name=tools.create_job_name(self._test_source_build or build,
                                       self._tag, test.name),
            control_type=test.test_type.capitalize(),
            meta_hosts=[self._board]*test.sync_count,
            dependencies=job_deps,
            keyvals=keyvals,
            max_runtime_mins=self._max_runtime_mins,
            timeout_mins=self._timeout_mins,
            parent_job_id=self._suite_job_id,
            test_retry=test.retries,
            priority=self._priority,
            synch_count=test.sync_count,
            require_ssp=test.require_ssp)

        setattr(test_obj, 'test_name', test.name)

        return test_obj


    def _schedule_test(self, record, test, retry_for=None, ignore_errors=False):
        """Schedule a single test and return the job.

        Schedule a single test by creating a job.
        And then update relevant data structures that are used to
        keep track of all running jobs.

        Emit TEST_NA if it failed to schedule the test due to
        NoEligibleHostException or a non-existent board label.

        @param record: A callable to use for logging.
                       prototype: record(base_job.status_log_entry)
        @param test: ControlData for a test to run.
        @param retry_for: If we are scheduling a test to retry an
                          old job, the afe_job_id of the old job
                          will be passed in as |retry_for|.
        @param ignore_errors: If True, when an rpc error occur, ignore
                             the error and will return None.
                             If False, rpc errors will be raised.

        @returns: A frontend.Job object if the test is successfully scheduled.
                  Returns None if scheduling failed due to
                  NoEligibleHostException or a non-existent board label.
                  Returns None if it encounters other rpc errors we don't know
                  how to handle and ignore_errors is False.

        """
        msg = 'Scheduling %s' % test.name
        if retry_for:
            msg = msg + ', to retry afe job %d' % retry_for
        logging.debug(msg)
        begin_time_str = datetime.datetime.now().strftime(time_utils.TIME_FMT)
        try:
            job = self._create_job(test, retry_for=retry_for)
        except error.NoEligibleHostException:
            logging.debug('%s not applicable for this board/pool. '
                          'Emitting TEST_NA.', test.name)
            Status('TEST_NA', test.name, 'Unsatisfiable DEPENDENCIES',
                   begin_time_str=begin_time_str).record_all(record)
        except proxy.ValidationError as e:
            # The goal here is to treat a dependency on a
            # non-existent board label the same as a
            # dependency on a board that exists, but for which
            # there's no hardware.
            #
            # As of this writing, the particular case we
            # want looks like this:
            #  1) e.problem_keys is a dictionary
            #  2) e.problem_keys['meta_hosts'] exists as
            #     the only key in the dictionary.
            #  3) e.problem_keys['meta_hosts'] matches this
            #     pattern: "Label "board:.*" not found"
            #
            # We check for conditions 1) and 2) on the
            # theory that they're relatively immutable.
            # We don't check condition 3) because it seems
            # likely to be a maintenance burden, and for the
            # times when we're wrong, being right shouldn't
            # matter enough (we _hope_).
            #
            # If we don't recognize the error, we pass
            # the buck to the outer try in this function,
            # which immediately fails the suite.
            if (not isinstance(e.problem_keys, dict) or
                    len(e.problem_keys) != 1 or
                    'meta_hosts' not in e.problem_keys):
                raise e
            logging.debug('Validation error: %s', str(e))
            logging.debug('Assuming label not found')
            Status('TEST_NA', test.name, e.problem_keys.values()[0],
                   begin_time_str=begin_time_str).record_all(record)
        except (error.RPCException, proxy.JSONRPCException) as e:
            if retry_for:
                # Mark that we've attempted to retry the old job.
                self._retry_handler.set_attempted(job_id=retry_for)
            if ignore_errors:
                logging.error('Failed to schedule test: %s, Reason: %s',
                              test.name, e)
            else:
                raise e
        else:
            self._jobs.append(job)
            self._jobs_to_tests[job.id] = test
            if retry_for:
                # A retry job was just created, record it.
                self._retry_handler.add_retry(
                        old_job_id=retry_for, new_job_id=job.id)
                retry_count = (test.job_retries -
                               self._retry_handler.get_retry_max(job.id))
                logging.debug('Job %d created to retry job %d. '
                              'Have retried for %d time(s)',
                              job.id, retry_for, retry_count)
            if self._results_dir:
                self._remember_provided_job_id(job)
            return job
        return None


    def schedule(self, record, add_experimental=True):
        #pylint: disable-msg=C0111
        """
        Schedule jobs using |self._afe|.

        frontend.Job objects representing each scheduled job will be put in
        |self._jobs|.

        @param record: A callable to use for logging.
                       prototype: record(base_job.status_log_entry)
        @param add_experimental: schedule experimental tests as well, or not.
        @returns: The number of tests that were scheduled.
        """
        logging.debug('Discovered %d stable tests.', len(self.stable_tests()))
        logging.debug('Discovered %d unstable tests.',
                      len(self.unstable_tests()))
        n_scheduled = 0

        Status('INFO', 'Start %s' % self._tag).record_result(record)
        try:
            tests = self.stable_tests()
            if add_experimental:
                for test in self.unstable_tests():
                    if not test.name.startswith(constants.EXPERIMENTAL_PREFIX):
                        test.name = constants.EXPERIMENTAL_PREFIX + test.name
                    tests.append(test)

            for test in tests:
                if self._schedule_test(record, test):
                    n_scheduled += 1
        except Exception:  # pylint: disable=W0703
            logging.error(traceback.format_exc())
            Status('FAIL', self._tag,
                   'Exception while scheduling suite').record_result(record)

        if self._job_retry:
            self._retry_handler = RetryHandler(
                    initial_jobs_to_tests=self._jobs_to_tests,
                    max_retries=self._max_retries)
        return n_scheduled


    def should_report(self, result):
        """
        Returns True if this failure requires to be reported.

        @param result: A result, encapsulating the status of the failed job.
        @return: True if we should report this failure.
        """
        if self._job_retry and self._retry_handler.has_following_retry(result):
            return False

        is_not_experimental = (
            constants.EXPERIMENTAL_PREFIX not in result._test_name and
            constants.EXPERIMENTAL_PREFIX not in result._job_name)

        return (self._file_bugs and result.test_executed and
                (is_not_experimental or self._file_experimental_bugs) and
                not result.is_testna() and
                result.is_worse_than(job_status.Status('GOOD', '', 'reason')))


    def wait(self, record, bug_template={}):
        """
        Polls for the job statuses, using |record| to print status when each
        completes.

        @param record: callable that records job status.
                 prototype:
                   record(base_job.status_log_entry)
        @param bug_template: A template dictionary specifying the default bug
                             filing options for failures in this suite.
        """
        if self._file_bugs:
            bug_reporter = reporting.Reporter()
        try:
            if self._suite_job_id:
                results_generator = job_status.wait_for_child_results(
                        self._afe, self._tko, self._suite_job_id)
            else:
                logging.warning('Unknown suite_job_id, falling back to less '
                             'efficient results_generator.')
                results_generator = job_status.wait_for_results(self._afe,
                                                                self._tko,
                                                                self._jobs)
            template = reporting_utils.BugTemplate(bug_template)
            for result in results_generator:
                result.record_all(record)
                if (self._results_dir and
                    job_status.is_for_infrastructure_fail(result)):
                    self._remember_provided_job_id(result)
                elif (self._results_dir and isinstance(result, Status)):
                    self._remember_test_status_job_id(result)

                if self._job_retry and self._retry_handler.should_retry(result):
                    new_job = self._schedule_test(
                            record=record, test=self._jobs_to_tests[result.id],
                            retry_for=result.id, ignore_errors=True)
                    if new_job:
                        results_generator.send([new_job])

                # TODO (fdeng): If the suite times out before a retry could
                # finish, we would lose the chance to file a bug for the
                # original job.
                if self.should_report(result):
                    job_views = self._tko.run('get_detailed_test_views',
                                              afe_job_id=result.id)
                    # Use the CrOS build for bug filing. If CrOS build is not
                    # specified, use the first build in the builds dictionary.
                    build = self._builds.get(provision.CROS_VERSION_PREFIX,
                                             self._builds.values()[0])
                    failure = reporting.TestBug(build,
                            site_utils.get_chrome_version(job_views),
                            self._tag,
                            result)

                    # Try to merge with bug template in test control file.
                    try:
                        test_data = self._jobs_to_tests[result.id]
                        merged_template = template.finalize_bug_template(
                                test_data.bug_template)
                    except AttributeError:
                        # Test control file does not have bug template defined.
                        merged_template = bug_template
                    except reporting_utils.InvalidBugTemplateException as e:
                        merged_template = {}
                        logging.error('Merging bug templates failed with '
                                      'error: %s An empty bug template will '
                                      'be used.', e)

                    # File bug when failure is one of the _FILE_BUG_SUITES,
                    # otherwise send an email to the owner anc cc.
                    if self._tag in _FILE_BUG_SUITES:
                        bug_id, bug_count = bug_reporter.report(failure,
                                                                merged_template)

                        # We use keyvals to communicate bugs filed with
                        # run_suite.
                        if bug_id is not None:
                            bug_keyvals = tools.create_bug_keyvals(
                                    result.id, result.test_name,
                                    (bug_id, bug_count))
                            try:
                                utils.write_keyval(self._results_dir,
                                                   bug_keyvals)
                            except ValueError:
                                logging.error('Unable to log bug keyval for:%s',
                                              result.test_name)
                    else:
                        reporting.send_email(failure, merged_template)

        except Exception:  # pylint: disable=W0703
            logging.error(traceback.format_exc())
            Status('FAIL', self._tag,
                   'Exception waiting for results').record_result(record)


    def abort(self):
        """
        Abort all scheduled test jobs.
        """
        if self._jobs:
            job_ids = [job.id for job in self._jobs]
            self._afe.run('abort_host_queue_entries', job__id__in=job_ids)


    def _remember_provided_job_id(self, job):
        """
        Record provided job as a suite job keyval, for later referencing.

        @param job: some representation of a job, including id, test_name
                    and owner
        """
        if job.id and job.owner and job.test_name:
            job_id_owner = '%s-%s' % (job.id, job.owner)
            logging.debug('Adding job keyval for %s=%s',
                          job.test_name, job_id_owner)
            utils.write_keyval(
                self._results_dir,
                {hashlib.md5(job.test_name).hexdigest(): job_id_owner})


    def _remember_test_status_job_id(self, status):
        """
        Record provided status as a test status keyval, for later referencing.

        @param status: Test status, including properties such as id, test_name
                       and owner.
        """
        if status.id and status.owner and status.test_name:
            test_id_owner = '%s-%s' % (status.id, status.owner)
            logging.debug('Adding status keyval for %s=%s',
                          status.test_name, test_id_owner)
            utils.write_keyval(
                self._results_dir,
                {hashlib.md5(status.test_name).hexdigest(): test_id_owner})


    @staticmethod
    def find_all_tests(cf_getter, suite_name='', add_experimental=False,
                       forgiving_parser=True, run_prod_code=False):
        """
        Function to scan through all tests and find all tests.

        Looks at control files returned by _cf_getter.get_control_file_list()
        for tests that pass self._predicate(). When this method is called
        with a file system ControlFileGetter, it performs a full parse of the
        root directory associated with the getter. This is the case when it's
        invoked from suite_preprocessor. When it's invoked with a devserver
        getter it looks up the suite_name in a suite to control file map
        generated at build time, and parses the relevant control files alone.
        This lookup happens on the devserver, so as far as this method is
        concerned, both cases are equivalent.

        @param cf_getter: a control_file_getter.ControlFileGetter used to list
               and fetch the content of control files
        @param suite_name: If specified, this method will attempt to restrain
                           the search space to just this suite's control files.
        @param add_experimental: add tests with experimental attribute set.
        @param forgiving_parser: If False, will raise ControlVariableExceptions
                                 if any are encountered when parsing control
                                 files. Note that this can raise an exception
                                 for syntax errors in unrelated files, because
                                 we parse them before applying the predicate.
        @param run_prod_code: If true, the suite will run the test code that
                              lives in prod aka the test code currently on the
                              lab servers by disabling SSP for the discovered
                              tests.

        @raises ControlVariableException: If forgiving_parser is False and there
                                          is a syntax error in a control file.

        @returns a dictionary of ControlData objects that based on given
                 parameters.
        """
        logging.debug('Getting control file list for suite: %s', suite_name)
        tests = {}
        files = cf_getter.get_control_file_list(suite_name=suite_name)

        logging.debug('Parsing control files ...')
        matcher = re.compile(r'[^/]+/(deps|profilers)/.+')
        for file in filter(lambda f: not matcher.match(f), files):
            text = cf_getter.get_control_file_contents(file)
            try:
                found_test = control_data.parse_control_string(
                        text, raise_warnings=True)
                if not add_experimental and found_test.experimental:
                    continue
                found_test.text = text
                found_test.path = file
                if run_prod_code:
                    found_test.require_ssp = False
                tests[file] = found_test
            except control_data.ControlVariableException, e:
                if not forgiving_parser:
                    msg = "Failed parsing %s\n%s" % (file, e)
                    raise control_data.ControlVariableException(msg)
                logging.warning("Skipping %s\n%s", file, e)
            except Exception, e:
                logging.error("Bad %s\n%s", file, e)
        return tests


    @staticmethod
    def find_and_parse_tests(cf_getter, predicate, suite_name='',
                             add_experimental=False, forgiving_parser=True,
                             run_prod_code=False):
        """
        Function to scan through all tests and find eligible tests.

        Search through all tests based on given cf_getter, suite_name,
        add_experimental and forgiving_parser, return the tests that match
        given predicate.

        @param cf_getter: a control_file_getter.ControlFileGetter used to list
               and fetch the content of control files
        @param predicate: a function that should return True when run over a
               ControlData representation of a control file that should be in
               this Suite.
        @param suite_name: If specified, this method will attempt to restrain
                           the search space to just this suite's control files.
        @param add_experimental: add tests with experimental attribute set.
        @param forgiving_parser: If False, will raise ControlVariableExceptions
                                 if any are encountered when parsing control
                                 files. Note that this can raise an exception
                                 for syntax errors in unrelated files, because
                                 we parse them before applying the predicate.
        @param run_prod_code: If true, the suite will run the test code that
                              lives in prod aka the test code currently on the
                              lab servers by disabling SSP for the discovered
                              tests.

        @raises ControlVariableException: If forgiving_parser is False and there
                                          is a syntax error in a control file.

        @return list of ControlData objects that should be run, with control
                file text added in |text| attribute. Results are sorted based
                on the TIME setting in control file, slowest test comes first.
        """
        tests = Suite.find_all_tests(cf_getter, suite_name, add_experimental,
                                     forgiving_parser,
                                     run_prod_code=run_prod_code)
        logging.debug('Parsed %s control files.', len(tests))
        tests = [test for test in tests.itervalues() if predicate(test)]
        tests.sort(key=lambda t:
                   control_data.ControlData.get_test_time_index(t.time),
                   reverse=True)
        return tests


    @staticmethod
    def find_possible_tests(cf_getter, predicate, suite_name='', count=10):
        """
        Function to scan through all tests and find possible tests.

        Search through all tests based on given cf_getter, suite_name,
        add_experimental and forgiving_parser. Use the given predicate to
        calculate the similarity and return the top 10 matches.

        @param cf_getter: a control_file_getter.ControlFileGetter used to list
               and fetch the content of control files
        @param predicate: a function that should return a tuple of (name, ratio)
               when run over a ControlData representation of a control file that
               should be in this Suite. `name` is the key to be compared, e.g.,
               a suite name or test name. `ratio` is a value between [0,1]
               indicating the similarity of `name` and the value to be compared.
        @param suite_name: If specified, this method will attempt to restrain
                           the search space to just this suite's control files.
        @param count: Number of suggestions to return, default to 10.

        @return list of top names that similar to the given test, sorted by
                match ratio.
        """
        tests = Suite.find_all_tests(cf_getter, suite_name,
                                     add_experimental=True,
                                     forgiving_parser=True)
        logging.debug('Parsed %s control files.', len(tests))
        similarities = {}
        for test in tests.itervalues():
            ratios = predicate(test)
            # Some predicates may return a list of tuples, e.g.,
            # name_in_tag_similarity_predicate. Convert all returns to a list.
            if not isinstance(ratios, list):
                ratios = [ratios]
            for name, ratio in ratios:
                similarities[name] = ratio
        return [s[0] for s in
                sorted(similarities.items(), key=operator.itemgetter(1),
                       reverse=True)][:count]
