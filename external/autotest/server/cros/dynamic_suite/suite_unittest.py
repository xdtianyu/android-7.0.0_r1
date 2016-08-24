#!/usr/bin/python
#
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


"""Unit tests for server/cros/dynamic_suite/dynamic_suite.py."""

import collections
import mox
import os
import shutil
import tempfile
import unittest
from collections import OrderedDict

import common

from autotest_lib.client.common_lib import base_job, control_data
from autotest_lib.client.common_lib import priorities
from autotest_lib.client.common_lib import utils, error
from autotest_lib.client.common_lib.cros import dev_server
from autotest_lib.server.cros import provision
from autotest_lib.server.cros.dynamic_suite import control_file_getter
from autotest_lib.server.cros.dynamic_suite import job_status
from autotest_lib.server.cros.dynamic_suite import reporting
from autotest_lib.server.cros.dynamic_suite.comparators import StatusContains
from autotest_lib.server.cros.dynamic_suite.suite import Suite
from autotest_lib.server.cros.dynamic_suite.suite import RetryHandler
from autotest_lib.server.cros.dynamic_suite.fakes import FakeControlData
from autotest_lib.server.cros.dynamic_suite.fakes import FakeJob
from autotest_lib.server import frontend
from autotest_lib.site_utils import phapi_lib


class SuiteTest(mox.MoxTestBase):
    """Unit tests for dynamic_suite Suite class.

    @var _BUILDS: fake build
    @var _TAG: fake suite tag
    """

    _BOARD = 'board:board'
    _BUILDS = {provision.CROS_VERSION_PREFIX:'build_1',
               provision.FW_RW_VERSION_PREFIX:'fwrw_build_1'}
    _TAG = 'au'
    _ATTR = {'attr:attr'}
    _DEVSERVER_HOST = 'http://dontcare:8080'
    _FAKE_JOB_ID = 10


    def setUp(self):
        super(SuiteTest, self).setUp()
        self.maxDiff = None
        self.afe = self.mox.CreateMock(frontend.AFE)
        self.tko = self.mox.CreateMock(frontend.TKO)

        self.tmpdir = tempfile.mkdtemp(suffix=type(self).__name__)

        self.getter = self.mox.CreateMock(control_file_getter.ControlFileGetter)
        self.devserver = dev_server.ImageServer(self._DEVSERVER_HOST)

        self.files = OrderedDict(
                [('one', FakeControlData(self._TAG, self._ATTR, 'data_one',
                                         'FAST', expr=True)),
                 ('two', FakeControlData(self._TAG, self._ATTR, 'data_two',
                                         'SHORT', dependencies=['feta'])),
                 ('three', FakeControlData(self._TAG, self._ATTR, 'data_three',
                                           'MEDIUM')),
                 ('four', FakeControlData('other', self._ATTR, 'data_four',
                                          'LONG', dependencies=['arugula'])),
                 ('five', FakeControlData(self._TAG, {'other'}, 'data_five',
                                          'LONG', dependencies=['arugula',
                                                                'caligula'])),
                 ('six', FakeControlData(self._TAG, self._ATTR, 'data_six',
                                         'LENGTHY')),
                 ('seven', FakeControlData(self._TAG, self._ATTR, 'data_seven',
                                           'FAST', job_retries=1))])

        self.files_to_filter = {
            'with/deps/...': FakeControlData(self._TAG, self._ATTR,
                                             'gets filtered'),
            'with/profilers/...': FakeControlData(self._TAG, self._ATTR,
                                                  'gets filtered')}


    def tearDown(self):
        super(SuiteTest, self).tearDown()
        shutil.rmtree(self.tmpdir, ignore_errors=True)


    def expect_control_file_parsing(self, suite_name=_TAG):
        """Expect an attempt to parse the 'control files' in |self.files|.

        @param suite_name: The suite name to parse control files for.
        """
        all_files = self.files.keys() + self.files_to_filter.keys()
        self._set_control_file_parsing_expectations(False, all_files,
                                                    self.files, suite_name)


    def _set_control_file_parsing_expectations(self, already_stubbed,
                                               file_list, files_to_parse,
                                               suite_name):
        """Expect an attempt to parse the 'control files' in |files|.

        @param already_stubbed: parse_control_string already stubbed out.
        @param file_list: the files the dev server returns
        @param files_to_parse: the {'name': FakeControlData} dict of files we
                               expect to get parsed.
        """
        if not already_stubbed:
            self.mox.StubOutWithMock(control_data, 'parse_control_string')

        self.getter.get_control_file_list(
            suite_name=suite_name).AndReturn(file_list)
        for file, data in files_to_parse.iteritems():
            self.getter.get_control_file_contents(
                file).InAnyOrder().AndReturn(data.string)
            control_data.parse_control_string(
                data.string, raise_warnings=True).InAnyOrder().AndReturn(data)


    def testFindAndParseStableTests(self):
        """Should find only non-experimental tests that match a predicate."""
        self.expect_control_file_parsing()
        self.mox.ReplayAll()

        predicate = lambda d: d.text == self.files['two'].string
        tests = Suite.find_and_parse_tests(self.getter, predicate, self._TAG)
        self.assertEquals(len(tests), 1)
        self.assertEquals(tests[0], self.files['two'])


    def testFindSuiteSyntaxErrors(self):
        """Check all control files for syntax errors.

        This test actually parses all control files in the autotest directory
        for syntax errors, by using the un-forgiving parser and pretending to
        look for all control files with the suite attribute.
        """
        autodir = os.path.abspath(
            os.path.join(os.path.dirname(__file__), '..', '..', '..'))
        fs_getter = Suite.create_fs_getter(autodir)
        predicate = lambda t: hasattr(t, 'suite')
        Suite.find_and_parse_tests(fs_getter, predicate, add_experimental=True,
                                   forgiving_parser=False)


    def testFindAndParseTestsSuite(self):
        """Should find all tests that match a predicate."""
        self.expect_control_file_parsing()
        self.mox.ReplayAll()

        predicate = lambda d: d.suite == self._TAG
        tests = Suite.find_and_parse_tests(self.getter,
                                           predicate,
                                           self._TAG,
                                           add_experimental=True)
        self.assertEquals(len(tests), 6)
        self.assertTrue(self.files['one'] in tests)
        self.assertTrue(self.files['two'] in tests)
        self.assertTrue(self.files['three'] in tests)
        self.assertTrue(self.files['five'] in tests)
        self.assertTrue(self.files['six'] in tests)
        self.assertTrue(self.files['seven'] in tests)


    def testFindAndParseTestsAttr(self):
        """Should find all tests that match a predicate."""
        self.expect_control_file_parsing()
        self.mox.ReplayAll()

        predicate = Suite.matches_attribute_expression_predicate('attr:attr')
        tests = Suite.find_and_parse_tests(self.getter,
                                           predicate,
                                           self._TAG,
                                           add_experimental=True)
        self.assertEquals(len(tests), 6)
        self.assertTrue(self.files['one'] in tests)
        self.assertTrue(self.files['two'] in tests)
        self.assertTrue(self.files['three'] in tests)
        self.assertTrue(self.files['four'] in tests)
        self.assertTrue(self.files['six'] in tests)
        self.assertTrue(self.files['seven'] in tests)


    def testAdHocSuiteCreation(self):
        """Should be able to schedule an ad-hoc suite by specifying
        a single test name."""
        self.expect_control_file_parsing(suite_name='ad_hoc_suite')
        self.mox.ReplayAll()
        predicate = Suite.test_name_equals_predicate('name-data_five')
        suite = Suite.create_from_predicates([predicate], self._BUILDS,
                                       self._BOARD, devserver=None,
                                       cf_getter=self.getter,
                                       afe=self.afe, tko=self.tko)

        self.assertFalse(self.files['one'] in suite.tests)
        self.assertFalse(self.files['two'] in suite.tests)
        self.assertFalse(self.files['one'] in suite.unstable_tests())
        self.assertFalse(self.files['two'] in suite.stable_tests())
        self.assertFalse(self.files['one'] in suite.stable_tests())
        self.assertFalse(self.files['two'] in suite.unstable_tests())
        self.assertFalse(self.files['four'] in suite.tests)
        self.assertTrue(self.files['five'] in suite.tests)


    def testStableUnstableFilter(self):
        """Should distinguish between experimental and stable tests."""
        self.expect_control_file_parsing()
        self.mox.ReplayAll()
        suite = Suite.create_from_name(self._TAG, self._BUILDS, self._BOARD,
                                       devserver=None,
                                       cf_getter=self.getter,
                                       afe=self.afe, tko=self.tko)

        self.assertTrue(self.files['one'] in suite.tests)
        self.assertTrue(self.files['two'] in suite.tests)
        self.assertTrue(self.files['one'] in suite.unstable_tests())
        self.assertTrue(self.files['two'] in suite.stable_tests())
        self.assertFalse(self.files['one'] in suite.stable_tests())
        self.assertFalse(self.files['two'] in suite.unstable_tests())
        # Sanity check.
        self.assertFalse(self.files['four'] in suite.tests)


    def mock_control_file_parsing(self):
        """Fake out find_and_parse_tests(), returning content from |self.files|.
        """
        for test in self.files.values():
            test.text = test.string  # mimic parsing.
        self.mox.StubOutWithMock(Suite, 'find_and_parse_tests')
        Suite.find_and_parse_tests(
            mox.IgnoreArg(),
            mox.IgnoreArg(),
            mox.IgnoreArg(),
            add_experimental=True,
            forgiving_parser=True,
            run_prod_code=False).AndReturn(self.files.values())


    def expect_job_scheduling(self, recorder, add_experimental,
                              tests_to_skip=[], ignore_deps=False,
                              raises=False, suite_deps=[], suite=None):
        """Expect jobs to be scheduled for 'tests' in |self.files|.

        @param add_experimental: expect jobs for experimental tests as well.
        @param recorder: object with a record_entry to be used to record test
                         results.
        @param tests_to_skip: [list, of, test, names] that we expect to skip.
        @param ignore_deps: If true, ignore tests' dependencies.
        @param raises: If True, expect exceptions.
        @param suite_deps: If True, add suite level dependencies.
        """
        record_job_id = suite and suite._results_dir
        if record_job_id:
            self.mox.StubOutWithMock(suite, '_remember_provided_job_id')
        recorder.record_entry(
            StatusContains.CreateFromStrings('INFO', 'Start %s' % self._TAG),
            log_in_subdir=False)
        tests = self.files.values()
        tests.sort(key=lambda test: test.experimental)
        n = 1
        for test in tests:
            if not add_experimental and test.experimental:
                continue
            if test.name in tests_to_skip:
                continue
            dependencies = []
            if not ignore_deps:
                dependencies.extend(test.dependencies)
            if suite_deps:
                dependencies.extend(suite_deps)
            dependencies.append(self._BOARD)
            build = self._BUILDS[provision.CROS_VERSION_PREFIX]
            job_mock = self.afe.create_job(
                control_file=test.text,
                name=mox.And(mox.StrContains(build),
                             mox.StrContains(test.name)),
                control_type=mox.IgnoreArg(),
                meta_hosts=[self._BOARD],
                dependencies=dependencies,
                keyvals={'build': build, 'suite': self._TAG,
                         'builds': SuiteTest._BUILDS,
                         'experimental':test.experimental},
                max_runtime_mins=24*60,
                timeout_mins=1440,
                parent_job_id=None,
                test_retry=0,
                priority=priorities.Priority.DEFAULT,
                synch_count=test.sync_count,
                require_ssp=test.require_ssp
                )
            if raises:
                job_mock.AndRaise(error.NoEligibleHostException())
                recorder.record_entry(
                        StatusContains.CreateFromStrings('START', test.name),
                        log_in_subdir=False)
                recorder.record_entry(
                        StatusContains.CreateFromStrings('TEST_NA', test.name),
                        log_in_subdir=False)
                recorder.record_entry(
                        StatusContains.CreateFromStrings('END', test.name),
                        log_in_subdir=False)
            else:
                fake_job = FakeJob(id=n)
                job_mock.AndReturn(fake_job)
                if record_job_id:
                    suite._remember_provided_job_id(fake_job)
                n += 1


    def testScheduleTestsAndRecord(self):
        """Should schedule stable and experimental tests with the AFE."""
        self.mock_control_file_parsing()
        self.mox.ReplayAll()
        suite = Suite.create_from_name(self._TAG, self._BUILDS, self._BOARD,
                                       self.devserver,
                                       afe=self.afe, tko=self.tko,
                                       results_dir=self.tmpdir)
        self.mox.ResetAll()
        recorder = self.mox.CreateMock(base_job.base_job)
        self.expect_job_scheduling(recorder, add_experimental=True, suite=suite)
        self.mox.ReplayAll()
        suite.schedule(recorder.record_entry, True)
        for job in suite._jobs:
            self.assertTrue(hasattr(job, 'test_name'))


    def testScheduleStableTests(self):
        """Should schedule only stable tests with the AFE."""
        self.mock_control_file_parsing()
        recorder = self.mox.CreateMock(base_job.base_job)
        self.expect_job_scheduling(recorder, add_experimental=False)

        self.mox.ReplayAll()
        suite = Suite.create_from_name(self._TAG, self._BUILDS, self._BOARD,
                                       self.devserver,
                                       afe=self.afe, tko=self.tko)
        suite.schedule(recorder.record_entry, add_experimental=False)


    def testScheduleStableTestsIgnoreDeps(self):
        """Should schedule only stable tests with the AFE."""
        self.mock_control_file_parsing()
        recorder = self.mox.CreateMock(base_job.base_job)
        self.expect_job_scheduling(recorder, add_experimental=False,
                                   ignore_deps=True)

        self.mox.ReplayAll()
        suite = Suite.create_from_name(self._TAG, self._BUILDS, self._BOARD,
                                       self.devserver,
                                       afe=self.afe, tko=self.tko,
                                       ignore_deps=True)
        suite.schedule(recorder.record_entry, add_experimental=False)


    def testScheduleUnrunnableTestsTESTNA(self):
        """Tests which fail to schedule should be TEST_NA."""
        self.mock_control_file_parsing()
        recorder = self.mox.CreateMock(base_job.base_job)
        self.expect_job_scheduling(recorder, add_experimental=True, raises=True)
        self.mox.ReplayAll()
        suite = Suite.create_from_name(self._TAG, self._BUILDS, self._BOARD,
                                       self.devserver,
                                       afe=self.afe, tko=self.tko)
        suite.schedule(recorder.record_entry, add_experimental=True)


    def testRetryMapAfterScheduling(self):
        """Test job-test and test-job mapping are correctly updated."""
        self.mock_control_file_parsing()
        recorder = self.mox.CreateMock(base_job.base_job)
        self.expect_job_scheduling(recorder, add_experimental=True)
        self.mox.ReplayAll()
        suite = Suite.create_from_name(self._TAG, self._BUILDS, self._BOARD,
                                       self.devserver,
                                       afe=self.afe, tko=self.tko,
                                       job_retry=True)
        suite.schedule(recorder.record_entry, add_experimental=True)
        all_files = self.files.items()
        # Sort tests in self.files so that they are in the same
        # order as they are scheduled.
        all_files.sort(key=lambda record: record[1].experimental)
        expected_retry_map = {}
        for n in range(len(all_files)):
             test = all_files[n][1]
             job_id = n + 1
             if test.job_retries > 0:
                 expected_retry_map[job_id] = {
                         'state': RetryHandler.States.NOT_ATTEMPTED,
                         'retry_max': test.job_retries}
        self.assertEqual(expected_retry_map, suite._retry_handler._retry_map)


    def testSuiteMaxRetries(self):
        self.mock_control_file_parsing()
        recorder = self.mox.CreateMock(base_job.base_job)
        self.expect_job_scheduling(recorder, add_experimental=True)
        self.mox.ReplayAll()
        suite = Suite.create_from_name(self._TAG, self._BUILDS, self._BOARD,
                                       self.devserver,
                                       afe=self.afe, tko=self.tko,
                                       job_retry=True, max_retries=1)
        suite.schedule(recorder.record_entry, add_experimental=True)
        self.assertEqual(suite._retry_handler._max_retries, 1)
        # Find the job_id of the test that allows retry
        job_id = suite._retry_handler._retry_map.iterkeys().next()
        suite._retry_handler.add_retry(old_job_id=job_id, new_job_id=10)
        self.assertEqual(suite._retry_handler._max_retries, 0)


    def testSuiteDependencies(self):
        """Should add suite dependencies to tests scheduled."""
        self.mock_control_file_parsing()
        recorder = self.mox.CreateMock(base_job.base_job)
        self.expect_job_scheduling(recorder, add_experimental=False,
                                   suite_deps=['extra'])

        self.mox.ReplayAll()
        suite = Suite.create_from_name(self._TAG, self._BUILDS, self._BOARD,
                                       self.devserver, extra_deps=['extra'],
                                       afe=self.afe, tko=self.tko)
        suite.schedule(recorder.record_entry, add_experimental=False)


    def _createSuiteWithMockedTestsAndControlFiles(self, file_bugs=False):
        """Create a Suite, using mocked tests and control file contents.

        @return Suite object, after mocking out behavior needed to create it.
        """
        self.expect_control_file_parsing()
        self.mox.ReplayAll()
        suite = Suite.create_from_name(self._TAG, self._BUILDS, self._BOARD,
                                       self.devserver,
                                       self.getter,
                                       afe=self.afe, tko=self.tko,
                                       file_bugs=file_bugs, job_retry=True)
        self.mox.ResetAll()
        return suite


    def _createSuiteMockResults(self, results_dir=None, will_file_bug=True,
                                result_status='FAIL'):
        """Create a suite, returned a set of mocked results to expect.

        @param results_dir: A mock results directory.
        @param will_file_bug: Whether later a bug will be filed.
                              If true, will mock out tko method.
        @param result_status: A desired result status, e.g. 'FAIL', 'WARN'.

        @return List of mocked results to wait on.
        """
        self.suite = self._createSuiteWithMockedTestsAndControlFiles(
                         file_bugs=True)
        self.suite._results_dir = results_dir
        test_report = self._get_bad_test_report(result_status)
        test_predicates = test_report.predicates
        test_fallout = test_report.fallout

        self.recorder = self.mox.CreateMock(base_job.base_job)
        self.recorder.record_entry = self.mox.CreateMock(
                base_job.base_job.record_entry)
        self._mock_recorder_with_results([test_predicates], self.recorder)
        if will_file_bug:
            self.suite._tko.run = self.mox.CreateMock(frontend.RpcClient.run)
            self.suite._tko.run('get_detailed_test_views',
                                afe_job_id=self._FAKE_JOB_ID)
        return [test_predicates, test_fallout]


    def _mock_recorder_with_results(self, results, recorder):
        """
        Checks that results are recoded in order, eg:
        START, (status, name, reason) END

        @param results: list of results
        @param recorder: status recorder
        """
        for result in results:
            status = result[0]
            test_name = result[1]
            recorder.record_entry(
                StatusContains.CreateFromStrings('START', test_name),
                log_in_subdir=False)
            recorder.record_entry(
                StatusContains.CreateFromStrings(*result),
                log_in_subdir=False).InAnyOrder('results')
            recorder.record_entry(
                StatusContains.CreateFromStrings('END %s' % status, test_name),
                log_in_subdir=False)


    def schedule_and_expect_these_results(self, suite, results, recorder):
        """Create mox stubs for call to suite.schedule and
        job_status.wait_for_results

        @param suite:    suite object for which to stub out schedule(...)
        @param results:  results object to be returned from
                         job_stats_wait_for_results(...)
        @param recorder: mocked recorder object to replay status messages
        """
        def result_generator(results):
            """A simple generator which generates results as Status objects.

            This generator handles 'send' by simply ignoring it.

            @param results: results object to be returned from
                            job_stats_wait_for_results(...)
            @yield: job_status.Status objects.
            """
            results = map(lambda r: job_status.Status(*r), results)
            for r in results:
                new_input = (yield r)
                if new_input:
                    yield None

        self.mox.StubOutWithMock(suite, 'schedule')
        suite.schedule(recorder.record_entry, True)
        suite._retry_handler = RetryHandler({})

        self.mox.StubOutWithMock(job_status, 'wait_for_results')
        job_status.wait_for_results(
                self.afe, self.tko, suite._jobs).AndReturn(
                result_generator(results))


    def testRunAndWaitSuccess(self):
        """Should record successful results."""
        suite = self._createSuiteWithMockedTestsAndControlFiles()

        recorder = self.mox.CreateMock(base_job.base_job)

        results = [('GOOD', 'good'), ('FAIL', 'bad', 'reason')]
        self._mock_recorder_with_results(results, recorder)
        self.schedule_and_expect_these_results(suite, results, recorder)
        self.mox.ReplayAll()

        suite.schedule(recorder.record_entry, True)
        suite.wait(recorder.record_entry)


    def testRunAndWaitFailure(self):
        """Should record failure to gather results."""
        suite = self._createSuiteWithMockedTestsAndControlFiles()

        recorder = self.mox.CreateMock(base_job.base_job)
        recorder.record_entry(
            StatusContains.CreateFromStrings('FAIL', self._TAG, 'waiting'),
            log_in_subdir=False)

        self.mox.StubOutWithMock(suite, 'schedule')
        suite.schedule(recorder.record_entry, True)
        self.mox.StubOutWithMock(job_status, 'wait_for_results')
        job_status.wait_for_results(mox.IgnoreArg(),
                                    mox.IgnoreArg(),
                                    mox.IgnoreArg()).AndRaise(
                                            Exception('Expected during test.'))
        self.mox.ReplayAll()

        suite.schedule(recorder.record_entry, True)
        suite.wait(recorder.record_entry)


    def testRunAndWaitScheduleFailure(self):
        """Should record failure to schedule jobs."""
        suite = self._createSuiteWithMockedTestsAndControlFiles()

        recorder = self.mox.CreateMock(base_job.base_job)
        recorder.record_entry(
            StatusContains.CreateFromStrings('INFO', 'Start %s' % self._TAG),
            log_in_subdir=False)

        recorder.record_entry(
            StatusContains.CreateFromStrings('FAIL', self._TAG, 'scheduling'),
            log_in_subdir=False)

        self.mox.StubOutWithMock(suite, '_create_job')
        suite._create_job(mox.IgnoreArg(), retry_for=mox.IgnoreArg()).AndRaise(
            Exception('Expected during test.'))
        self.mox.ReplayAll()

        suite.schedule(recorder.record_entry, True)
        suite.wait(recorder.record_entry)


    def testGetTestsSortedByTime(self):
        """Should find all tests and sorted by TIME setting."""
        self.expect_control_file_parsing()
        self.mox.ReplayAll()
        # Get all tests.
        tests = Suite.find_and_parse_tests(self.getter,
                                           lambda d: True,
                                           self._TAG,
                                           add_experimental=True)
        self.assertEquals(len(tests), 7)
        times = [control_data.ControlData.get_test_time_index(test.time)
                 for test in tests]
        self.assertTrue(all(x>=y for x, y in zip(times, times[1:])),
                        'Tests are not ordered correctly.')


    def _get_bad_test_report(self, result_status='FAIL'):
        """
        Fetch the predicates of a failing test, and the parameters
        that are a fallout of this test failing.
        """
        predicates = collections.namedtuple('predicates',
                                            'status, testname, reason')
        fallout = collections.namedtuple('fallout',
                                         ('time_start, time_end, job_id,'
                                          'username, hostname'))
        test_report = collections.namedtuple('test_report',
                                             'predicates, fallout')
        return test_report(predicates(result_status, 'bad_test',
                                      'dreadful_reason'),
                           fallout('2014-01-01 01:01:01', 'None',
                                   self._FAKE_JOB_ID, 'user', 'myhost'))


    def mock_bug_filing(self, test_results):
        """A helper function that mocks bug filing.

        @param test_results: A named tuple (predicates, fallout) representing
                             a bad test report.
        """
        def check_result(result):
            """
            Checks to see if the status passed to the bug reporter contains all
            the arguments required to file bugs.

            @param result: The result we get when a test fails.
            """
            test_predicates = test_results[0]
            test_fallout = test_results[1]
            expected_result = job_status.Status(
                test_predicates.status, test_predicates.testname,
                reason=test_predicates.reason,
                job_id=test_fallout.job_id, owner=test_fallout.username,
                hostname=test_fallout.hostname,
                begin_time_str=test_fallout.time_start)

            return all(getattr(result, k, None) == v for k, v in
                       expected_result.__dict__.iteritems()
                       if 'timestamp' not in str(k))

        self.mox.StubOutWithMock(reporting, 'TestBug')
        reporting.TestBug(self._BUILDS[provision.CROS_VERSION_PREFIX],
                          mox.IgnoreArg(), mox.IgnoreArg(),
                          mox.Func(check_result))

        self.mox.StubOutClassWithMocks(phapi_lib, 'ProjectHostingApiClient')
        mock_host = phapi_lib.ProjectHostingApiClient(mox.IgnoreArg(),
                                                      mox.IgnoreArg())
        self.mox.StubOutWithMock(reporting.Reporter, 'report')
        reporting.Reporter.report(mox.IgnoreArg(),
                                  mox.IgnoreArg()).AndReturn((0, 0))

        self.mox.StubOutWithMock(utils, 'write_keyval')
        utils.write_keyval(mox.IgnoreArg(), mox.IgnoreArg())


    def testBugFiling(self):
        """
        Confirm that all the necessary predicates are passed on to the
        bug reporter when a test fails.
        """
        test_results = self._createSuiteMockResults()
        self.schedule_and_expect_these_results(
            self.suite,
            [test_results[0] + test_results[1]],
            self.recorder)

        self.mock_bug_filing(test_results)
        self.mox.ReplayAll()

        self.suite.schedule(self.recorder.record_entry, True)
        self.suite._jobs_to_tests[self._FAKE_JOB_ID] = self.files['seven']
        self.suite.wait(self.recorder.record_entry)


    def testFailedBugFiling(self):
        """
        Make sure the suite survives even if we cannot file bugs.
        """
        test_results = self._createSuiteMockResults(self.tmpdir)
        self.schedule_and_expect_these_results(
            self.suite,
            [test_results[0] + test_results[1]],
            self.recorder)
        self.mox.StubOutWithMock(reporting.Reporter, '_check_tracker')
        self.mox.StubOutClassWithMocks(phapi_lib, 'ProjectHostingApiClient')
        mock_host = phapi_lib.ProjectHostingApiClient(mox.IgnoreArg(),
                                                      mox.IgnoreArg())
        reporting.Reporter._check_tracker().AndReturn(False)

        self.mox.ReplayAll()

        self.suite.schedule(self.recorder.record_entry, True)
        self.suite._jobs_to_tests[self._FAKE_JOB_ID] = self.files['seven']
        self.suite.wait(self.recorder.record_entry)


    def testJobRetryTestFail(self):
        """Test retry works."""
        test_to_retry = self.files['seven']
        fake_new_job_id = self._FAKE_JOB_ID + 1
        fake_job = FakeJob(id=self._FAKE_JOB_ID)
        fake_new_job = FakeJob(id=fake_new_job_id)

        test_results = self._createSuiteMockResults(will_file_bug=False)
        self.schedule_and_expect_these_results(
                self.suite,
                [test_results[0] + test_results[1]],
                self.recorder)
        self.mox.StubOutWithMock(self.suite, '_create_job')
        self.suite._create_job(
                test_to_retry,
                retry_for=self._FAKE_JOB_ID).AndReturn(fake_new_job)
        self.mox.ReplayAll()
        self.suite.schedule(self.recorder.record_entry, True)
        self.suite._retry_handler._retry_map = {
                self._FAKE_JOB_ID: {'state': RetryHandler.States.NOT_ATTEMPTED,
                                    'retry_max': 1}
                }
        self.suite._jobs_to_tests[self._FAKE_JOB_ID] = test_to_retry
        self.suite.wait(self.recorder.record_entry)
        expected_retry_map = {
                self._FAKE_JOB_ID: {'state': RetryHandler.States.RETRIED,
                                    'retry_max': 1},
                fake_new_job_id: {'state': RetryHandler.States.NOT_ATTEMPTED,
                                  'retry_max': 0}
                }
        # Check retry map is correctly updated
        self.assertEquals(self.suite._retry_handler._retry_map,
                          expected_retry_map)
        # Check _jobs_to_tests is correctly updated
        self.assertEquals(self.suite._jobs_to_tests[fake_new_job_id],
                          test_to_retry)


    def testJobRetryTestWarn(self):
        """Test that no retry is scheduled if test warns."""
        test_to_retry = self.files['seven']
        fake_job = FakeJob(id=self._FAKE_JOB_ID)
        test_results = self._createSuiteMockResults(
                will_file_bug=True, result_status='WARN')
        self.schedule_and_expect_these_results(
                self.suite,
                [test_results[0] + test_results[1]],
                self.recorder)
        # A bug should be filed if test warns.
        self.mock_bug_filing(test_results)
        self.mox.ReplayAll()
        self.suite.schedule(self.recorder.record_entry, True)
        self.suite._retry_handler._retry_map = {
                self._FAKE_JOB_ID: {'state': RetryHandler.States.NOT_ATTEMPTED,
                                    'retry_max': 1}
                }
        self.suite._jobs_to_tests[self._FAKE_JOB_ID] = test_to_retry
        expected_jobs_to_tests = self.suite._jobs_to_tests.copy()
        expected_retry_map = self.suite._retry_handler._retry_map.copy()
        self.suite.wait(self.recorder.record_entry)
        # Check retry map and _jobs_to_tests, ensure no retry was scheduled.
        self.assertEquals(self.suite._retry_handler._retry_map,
                          expected_retry_map)
        self.assertEquals(self.suite._jobs_to_tests, expected_jobs_to_tests)


    def testFailedJobRetry(self):
        """Make sure the suite survives even if the retry failed."""
        test_to_retry = self.files['seven']
        fake_job = FakeJob(id=self._FAKE_JOB_ID)

        test_results = self._createSuiteMockResults(will_file_bug=False)
        self.schedule_and_expect_these_results(
                self.suite,
                [test_results[0] + test_results[1]],
                self.recorder)
        self.mox.StubOutWithMock(self.suite, '_create_job')
        self.suite._create_job(
                test_to_retry, retry_for=self._FAKE_JOB_ID).AndRaise(
                error.RPCException('Expected during test'))
        # Do not file a bug.
        self.mox.StubOutWithMock(self.suite, 'should_report')
        self.suite.should_report(mox.IgnoreArg()).AndReturn(False)

        self.mox.ReplayAll()

        self.suite.schedule(self.recorder.record_entry, True)
        self.suite._retry_handler._retry_map = {
                self._FAKE_JOB_ID: {
                        'state': RetryHandler.States.NOT_ATTEMPTED,
                        'retry_max': 1}}
        self.suite._jobs_to_tests[self._FAKE_JOB_ID] = test_to_retry
        self.suite.wait(self.recorder.record_entry)
        expected_retry_map = {
                self._FAKE_JOB_ID: {
                        'state': RetryHandler.States.ATTEMPTED,
                        'retry_max': 1}}
        expected_jobs_to_tests = self.suite._jobs_to_tests.copy()
        self.assertEquals(self.suite._retry_handler._retry_map,
                          expected_retry_map)
        self.assertEquals(self.suite._jobs_to_tests, expected_jobs_to_tests)


if __name__ == '__main__':
    unittest.main()
