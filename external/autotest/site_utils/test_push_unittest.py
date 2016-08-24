#!/usr/bin/python
# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import StringIO
import mox
import unittest
import urllib2

import common
from autotest_lib.server import site_utils
from autotest_lib.server.cros.dynamic_suite import constants
from autotest_lib.server.cros.dynamic_suite import frontend_wrappers
from autotest_lib.server.cros.dynamic_suite import reporting
from autotest_lib.site_utils import phapi_lib
from autotest_lib.site_utils import test_push

AUTOFILED_COUNT_2 = '%s2' % reporting.Reporter.AUTOFILED_COUNT

class TestPushUnittests(mox.MoxTestBase):
    """Unittest for test_push script."""

    def setUp(self):
        """Initialize the unittest."""
        super(TestPushUnittests, self).setUp()
        # Overwrite expected test results.
        test_push.EXPECTED_TEST_RESULTS = {
            '^SERVER_JOB$':                  'GOOD',
            '.*control.dependency$':         'TEST_NA',
            '.*dummy_Fail.RetryFail$':       'FAIL',
            }


    def stub_out_methods(self, test_views):
        """Stub out methods in test_push module with given test results.

        @param test_views: Desired test result views.

        """
        self.mox.UnsetStubs()
        response = StringIO.StringIO('some_value')
        self.mox.StubOutWithMock(urllib2, 'urlopen')
        urllib2.urlopen(mox.IgnoreArg()).AndReturn(response)

        self.mox.StubOutWithMock(test_push, 'get_default_build')
        test_push.get_default_build(mox.IgnoreArg(), mox.IgnoreArg()).AndReturn(
                'stumpy-release/R36-5881-0.0')
        test_push.get_default_build(mox.IgnoreArg(), mox.IgnoreArg()).AndReturn(
                'quawks-release/R36-5881-0.0')

        self.mox.StubOutWithMock(test_push, 'check_dut_image')
        test_push.check_dut_image(mox.IgnoreArg(), mox.IgnoreArg()).AndReturn(
                None)

        self.mox.StubOutWithMock(test_push, 'do_run_suite')
        test_push.do_run_suite(test_push.PUSH_TO_PROD_SUITE, mox.IgnoreArg(),
                               mox.IgnoreArg(), mox.IgnoreArg()
                               ).AndReturn((1))

        self.mox.StubOutWithMock(site_utils, 'get_test_views_from_tko')
        self.mox.StubOutWithMock(frontend_wrappers, 'RetryingTKO')
        frontend_wrappers.RetryingTKO(timeout_min=0.1,
                                      delay_sec=10).AndReturn(None)
        site_utils.get_test_views_from_tko(1, None).AndReturn(test_views)


    def test_suite_success(self):
        """Test test_suite method with matching results."""
        test_views = {'SERVER_JOB':                        'GOOD',
                      'dummy_fail/control.dependency':     'TEST_NA',
                      'dummy_Fail.RetryFail':              'FAIL'
                      }

        self.stub_out_methods(test_views)
        self.mox.ReplayAll()
        test_push.test_suite(test_push.PUSH_TO_PROD_SUITE, test_views,
                             arguments=test_push.parse_arguments())
        self.mox.VerifyAll()


    def test_suite_fail_with_missing_test(self):
        """Test test_suite method that should fail with missing test."""
        test_views = {'SERVER_JOB':                        'GOOD',
                      'dummy_fail/control.dependency':     'TEST_NA',
                      }

        self.stub_out_methods(test_views)
        self.mox.ReplayAll()
        test_push.test_suite(test_push.PUSH_TO_PROD_SUITE, test_views,
                             arguments=test_push.parse_arguments())
        self.mox.VerifyAll()


    def test_suite_fail_with_unexpected_test_results(self):
        """Test test_suite method that should fail with unexpected test results.
        """
        test_views = {'SERVER_JOB':                        'FAIL',
                      'dummy_fail/control.dependency':     'TEST_NA',
                      'dummy_Fail.RetryFail':              'FAIL',
                      }

        self.stub_out_methods(test_views)
        self.mox.ReplayAll()
        test_push.test_suite(test_push.PUSH_TO_PROD_SUITE, test_views,
                             arguments=test_push.parse_arguments())
        self.mox.VerifyAll()


    def test_suite_fail_with_extra_test(self):
        """Test test_suite method that should fail with extra test."""
        test_views = {'SERVER_JOB':                        'GOOD',
                      'dummy_fail/control.dependency':     'TEST_NA',
                      'dummy_Fail.RetryFail':              'FAIL',
                      'dummy_Fail.ExtraTest':              'GOOD',
                      }

        self.stub_out_methods(test_views)
        self.mox.ReplayAll()
        test_push.test_suite(test_push.PUSH_TO_PROD_SUITE, test_views,
                             arguments=test_push.parse_arguments())
        self.mox.VerifyAll()


    def test_close_bug_fail(self):
        """Test close_bug method that failed to close a bug."""
        issue = self.mox.CreateMock(phapi_lib.Issue)
        issue.id = 100
        issue.labels = []
        issue.state = constants.ISSUE_OPEN

        self.mox.StubOutWithMock(reporting.Reporter, 'find_issue_by_marker')
        reporting.Reporter.find_issue_by_marker(mox.IgnoreArg()).AndReturn(
            issue)
        reporting.Reporter.find_issue_by_marker(mox.IgnoreArg()).AndReturn(
            issue)
        self.mox.StubOutWithMock(reporting.Reporter, 'modify_bug_report')
        reporting.Reporter.modify_bug_report(mox.IgnoreArg(),
                                             comment=mox.IgnoreArg(),
                                             label_update=mox.IgnoreArg(),
                                             status=mox.IgnoreArg()).AndReturn(
                                                                        None)
        self.mox.ReplayAll()
        self.assertRaises(test_push.TestPushException, test_push.close_bug)
        self.mox.VerifyAll()


    def test_check_bug_filed_and_deduped_fail_to_find_bug(self):
        """Test check_bug_filed_and_deduped method that failed to find a bug.
        """
        self.mox.StubOutWithMock(reporting.Reporter, 'find_issue_by_marker')
        reporting.Reporter.find_issue_by_marker(mox.IgnoreArg()).AndReturn(
            None)
        self.mox.ReplayAll()
        self.assertRaises(test_push.TestPushException,
                          test_push.check_bug_filed_and_deduped, None)
        self.mox.VerifyAll()


    def create_mock_issue(self, id, labels=[]):
        """Create a mock issue with given id and lables.

        @param id: id of the issue.
        @param labels: labels of the issue.

        """
        issue = self.mox.CreateMock(phapi_lib.Issue)
        issue.id = id
        issue.labels = labels
        issue.state = constants.ISSUE_OPEN
        return issue


    def test_check_bug_filed_and_deduped_fail_to_find_bug2(self):
        """Test check_bug_filed_and_deduped method that failed to find a bug.
        """
        issue = self.create_mock_issue(100)

        self.mox.StubOutWithMock(reporting.Reporter, 'find_issue_by_marker')
        reporting.Reporter.find_issue_by_marker(mox.IgnoreArg()).AndReturn(
            issue)
        self.mox.ReplayAll()
        self.assertRaises(test_push.TestPushException,
                          test_push.check_bug_filed_and_deduped, [100])
        self.mox.VerifyAll()


    def test_check_bug_filed_and_deduped_fail_to_dedupe(self):
        """Test check_bug_filed_and_deduped method that failed with dedupe.
        """
        issue = self.create_mock_issue(100)

        self.mox.StubOutWithMock(reporting.Reporter, 'find_issue_by_marker')
        reporting.Reporter.find_issue_by_marker(mox.IgnoreArg()).AndReturn(
            issue)
        self.mox.ReplayAll()
        self.assertRaises(test_push.TestPushException,
                          test_push.check_bug_filed_and_deduped, [99])
        self.mox.VerifyAll()


    def test_check_bug_filed_and_deduped_fail_more_than_1_bug(self):
        """Test check_bug_filed_and_deduped method that failed with finding
        more than one bug.
        """
        issue = self.create_mock_issue(100, [AUTOFILED_COUNT_2])
        second_issue = self.create_mock_issue(101)

        self.mox.StubOutWithMock(reporting.Reporter, 'find_issue_by_marker')
        reporting.Reporter.find_issue_by_marker(mox.IgnoreArg()).AndReturn(
            issue)
        reporting.Reporter.find_issue_by_marker(mox.IgnoreArg()).AndReturn(
            second_issue)
        self.mox.StubOutWithMock(reporting.Reporter, 'modify_bug_report')
        reporting.Reporter.modify_bug_report(mox.IgnoreArg(),
                                             comment=mox.IgnoreArg(),
                                             label_update=mox.IgnoreArg(),
                                             status=mox.IgnoreArg()
                                             ).AndReturn(None)
        self.mox.ReplayAll()
        self.assertRaises(test_push.TestPushException,
                          test_push.check_bug_filed_and_deduped, [99])
        self.mox.VerifyAll()


    def test_check_bug_filed_and_deduped_succeed_to_dedupe(self):
        """Test check_bug_filed_and_deduped method that succeeded with dedupe.
        """
        issue = self.create_mock_issue(100, [AUTOFILED_COUNT_2])

        self.mox.StubOutWithMock(reporting.Reporter, 'find_issue_by_marker')
        reporting.Reporter.find_issue_by_marker(mox.IgnoreArg()).AndReturn(
            issue)
        reporting.Reporter.find_issue_by_marker(mox.IgnoreArg()).AndReturn(
            None)
        self.mox.StubOutWithMock(reporting.Reporter, 'modify_bug_report')
        reporting.Reporter.modify_bug_report(mox.IgnoreArg(),
                                             comment=mox.IgnoreArg(),
                                             label_update=mox.IgnoreArg(),
                                             status=mox.IgnoreArg()
                                             ).AndReturn(None)
        self.mox.ReplayAll()
        test_push.check_bug_filed_and_deduped([99])
        self.mox.VerifyAll()


if __name__ == '__main__':
    unittest.main()
