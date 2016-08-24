#!/usr/bin/python
#
# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

import mox

import common
from autotest_lib.server.cros.dynamic_suite import constants
from autotest_lib.server.cros.dynamic_suite import job_status
from autotest_lib.server.cros.dynamic_suite import reporting
from autotest_lib.server.cros.dynamic_suite import reporting_utils
from autotest_lib.server.cros.dynamic_suite import tools
from autotest_lib.site_utils import phapi_lib
from chromite.lib import gdata_lib


class ReportingTest(mox.MoxTestBase):
    """Unittests to verify basic control flow for automatic bug filing."""

    # fake issue id to use in testing duplicate issues
    _FAKE_ISSUE_ID = 123

    # test report used to generate failure
    test_report = {
        'build':'build-build/R1-1',
        'chrome_version':'28.0',
        'suite':'suite',
        'test':'bad_test',
        'reason':'dreadful_reason',
        'owner':'user',
        'hostname':'myhost',
        'job_id':'myjob',
        'status': 'FAIL',
    }

    bug_template = {
        'labels': ['Cr-Internals-WebRTC'],
        'owner': 'myself',
        'status': 'Fixed',
        'summary': 'This is a short summary',
        'title': None,
    }

    def _get_failure(self, is_server_job=False):
        """Get a TestBug so we can report it.

        @param is_server_job: Set to True of failed job is a server job. Server
                job's test name is formated as build/suite/test_name.
        @return: a failure object initialized with values from test_report.
        """
        if is_server_job:
            test_name = tools.create_job_name(
                    self.test_report.get('build'),
                    self.test_report.get('suite'),
                    self.test_report.get('test'))
        else:
            test_name = self.test_report.get('test')
        expected_result = job_status.Status(self.test_report.get('status'),
            test_name,
            reason=self.test_report.get('reason'),
            job_id=self.test_report.get('job_id'),
            owner=self.test_report.get('owner'),
            hostname=self.test_report.get('hostname'))

        return reporting.TestBug(self.test_report.get('build'),
            self.test_report.get('chrome_version'),
            self.test_report.get('suite'), expected_result)


    def setUp(self):
        super(ReportingTest, self).setUp()
        self.mox.StubOutClassWithMocks(phapi_lib, 'ProjectHostingApiClient')
        self._orig_project_name = reporting.Reporter._project_name

        # We want to have some data so that the Reporter doesn't fail at
        # initialization.
        reporting.Reporter._project_name = 'project'


    def tearDown(self):
        reporting.Reporter._project_name = self._orig_project_name
        super(ReportingTest, self).tearDown()


    def testNewIssue(self):
        """Add a new issue to the tracker when a matching issue isn't found.

        Confirms that we call CreateTrackerIssue when an Issue search
        returns None.
        """
        self.mox.StubOutWithMock(reporting.Reporter, 'find_issue_by_marker')
        self.mox.StubOutWithMock(reporting.TestBug, 'summary')

        client = phapi_lib.ProjectHostingApiClient(mox.IgnoreArg(),
                                                   mox.IgnoreArg())
        client.create_issue(mox.IgnoreArg()).AndReturn(
            {'id': self._FAKE_ISSUE_ID})
        reporting.Reporter.find_issue_by_marker(mox.IgnoreArg()).AndReturn(
            None)
        reporting.TestBug.summary().AndReturn('')

        self.mox.ReplayAll()
        bug_id, bug_count = reporting.Reporter().report(self._get_failure())

        self.assertEqual(bug_id, self._FAKE_ISSUE_ID)
        self.assertEqual(bug_count, 1)


    def testDuplicateIssue(self):
        """Dedupe to an existing issue when one is found.

        Confirms that we call AppendTrackerIssueById with the same issue
        returned by the issue search.
        """
        self.mox.StubOutWithMock(reporting.Reporter, 'find_issue_by_marker')
        self.mox.StubOutWithMock(reporting.TestBug, 'summary')

        issue = self.mox.CreateMock(phapi_lib.Issue)
        issue.id = self._FAKE_ISSUE_ID
        issue.labels = []
        issue.state = constants.ISSUE_OPEN

        client = phapi_lib.ProjectHostingApiClient(mox.IgnoreArg(),
                                                   mox.IgnoreArg())
        client.update_issue(self._FAKE_ISSUE_ID, mox.IgnoreArg())
        reporting.Reporter.find_issue_by_marker(mox.IgnoreArg()).AndReturn(
            issue)

        reporting.TestBug.summary().AndReturn('')

        self.mox.ReplayAll()
        bug_id, bug_count = reporting.Reporter().report(self._get_failure())

        self.assertEqual(bug_id, self._FAKE_ISSUE_ID)
        self.assertEqual(bug_count, 2)


    def testSuiteIssueConfig(self):
        """Test that the suite bug template values are not overridden."""

        def check_suite_options(issue):
            """
            Checks to see if the options specified in bug_template reflect in
            the issue we're about to file, and that the autofiled label was not
            lost in the process.

            @param issue: issue to check labels on.
            """
            assert('autofiled' in issue.labels)
            for k, v in self.bug_template.iteritems():
                if (isinstance(v, list)
                    and all(item in getattr(issue, k) for item in v)):
                    continue
                if v and getattr(issue, k) is not v:
                    return False
            return True

        self.mox.StubOutWithMock(reporting.Reporter, 'find_issue_by_marker')
        self.mox.StubOutWithMock(reporting.TestBug, 'summary')

        reporting.Reporter.find_issue_by_marker(mox.IgnoreArg()).AndReturn(
            None)
        reporting.TestBug.summary().AndReturn('Summary')

        mock_host = phapi_lib.ProjectHostingApiClient(mox.IgnoreArg(),
                                                      mox.IgnoreArg())
        mock_host.create_issue(mox.IgnoreArg()).AndReturn(
            {'id': self._FAKE_ISSUE_ID})

        self.mox.ReplayAll()
        bug_id, bug_count = reporting.Reporter().report(self._get_failure(),
                                                        self.bug_template)

        self.assertEqual(bug_id, self._FAKE_ISSUE_ID)
        self.assertEqual(bug_count, 1)


    def testGenericBugCanBeFiled(self):
        """Test that we can use a Bug object to file a bug report."""
        self.mox.StubOutWithMock(reporting.Reporter, 'find_issue_by_marker')

        bug = reporting.Bug('title', 'summary', 'marker')

        reporting.Reporter.find_issue_by_marker(mox.IgnoreArg()).AndReturn(
            None)

        mock_host = phapi_lib.ProjectHostingApiClient(mox.IgnoreArg(),
                                                      mox.IgnoreArg())
        mock_host.create_issue(mox.IgnoreArg()).AndReturn(
            {'id': self._FAKE_ISSUE_ID})

        self.mox.ReplayAll()
        bug_id, bug_count = reporting.Reporter().report(bug)

        self.assertEqual(bug_id, self._FAKE_ISSUE_ID)
        self.assertEqual(bug_count, 1)


    def testWithSearchMarkerSetToNoneIsNotDeduped(self):
        """Test that we do not dedupe bugs that have no search marker."""

        bug = reporting.Bug('title', 'summary', search_marker=None)

        mock_host = phapi_lib.ProjectHostingApiClient(mox.IgnoreArg(),
                                                      mox.IgnoreArg())
        mock_host.create_issue(mox.IgnoreArg()).AndReturn(
            {'id': self._FAKE_ISSUE_ID})

        self.mox.ReplayAll()
        bug_id, bug_count = reporting.Reporter().report(bug)

        self.assertEqual(bug_id, self._FAKE_ISSUE_ID)
        self.assertEqual(bug_count, 1)


    def testSearchMarkerNoBuildSuiteInfo(self):
        """Test that the search marker does not include build and suite info."""
        test_failure = self._get_failure(is_server_job=True)
        search_marker = test_failure.search_marker()
        self.assertFalse(test_failure.build in search_marker,
                         ('Build information should not be presented in search '
                          'marker.'))


class FindIssueByMarkerTests(mox.MoxTestBase):
    """Tests the find_issue_by_marker function."""

    def setUp(self):
        super(FindIssueByMarkerTests, self).setUp()
        self.mox.StubOutClassWithMocks(phapi_lib, 'ProjectHostingApiClient')
        self._orig_project_name = reporting.Reporter._project_name

        # We want to have some data so that the Reporter doesn't fail at
        # initialization.
        reporting.Reporter._project_name = 'project'


    def tearDown(self):
        reporting.Reporter._project_name = self._orig_project_name
        super(FindIssueByMarkerTests, self).tearDown()


    def testReturnNoneIfMarkerIsNone(self):
        """Test that we do not look up an issue if the search marker is None."""
        mock_host = phapi_lib.ProjectHostingApiClient(mox.IgnoreArg(),
                                                      mox.IgnoreArg())

        self.mox.ReplayAll()
        result = reporting.Reporter().find_issue_by_marker(None)
        self.assertTrue(result is None)


class AnchorSummaryTests(mox.MoxTestBase):
    """Tests the _anchor_summary function."""

    def setUp(self):
        super(AnchorSummaryTests, self).setUp()
        self.mox.StubOutClassWithMocks(phapi_lib, 'ProjectHostingApiClient')
        self._orig_project_name = reporting.Reporter._project_name

        # We want to have some data so that the Reporter doesn't fail at
        # initialization.
        reporting.Reporter._project_name = 'project'


    def tearDown(self):
        reporting.Reporter._project_name = self._orig_project_name
        super(AnchorSummaryTests, self).tearDown()


    def test_summary_returned_untouched_if_no_search_maker(self):
        """Test that we just return the summary if we have no search marker."""
        mock_host = phapi_lib.ProjectHostingApiClient(mox.IgnoreArg(),
                                                      mox.IgnoreArg())

        bug = reporting.Bug('title', 'summary', None)

        self.mox.ReplayAll()
        result = reporting.Reporter()._anchor_summary(bug)

        self.assertEqual(result, 'summary')


    def test_append_anchor_to_summary_if_search_marker(self):
        """Test that we add an anchor to the search marker."""
        mock_host = phapi_lib.ProjectHostingApiClient(mox.IgnoreArg(),
                                                      mox.IgnoreArg())

        bug = reporting.Bug('title', 'summary', 'marker')

        self.mox.ReplayAll()
        result = reporting.Reporter()._anchor_summary(bug)

        self.assertEqual(result, 'summary\n\n%smarker\n' %
                                 reporting.Reporter._SEARCH_MARKER)


class LabelUpdateTests(mox.MoxTestBase):
    """Test the _create_autofiled_count_update() function."""

    def setUp(self):
        super(LabelUpdateTests, self).setUp()
        self.mox.StubOutClassWithMocks(phapi_lib, 'ProjectHostingApiClient')
        self._orig_project_name = reporting.Reporter._project_name

        # We want to have some data so that the Reporter doesn't fail at
        # initialization.
        reporting.Reporter._project_name = 'project'


    def tearDown(self):
        reporting.Reporter._project_name = self._orig_project_name
        super(LabelUpdateTests, self).tearDown()


    def _create_count_label(self, n):
        return '%s%d' % (reporting.Reporter.AUTOFILED_COUNT, n)


    def _test_count_label_update(self, labels, remove, expected_count):
        """Utility to test _create_autofiled_count_update().

        @param labels         Input list of labels.
        @param remove         List of labels expected to be removed
                              in the result.
        @param expected_count Count value expected to be returned
                              from the call.
        """
        client = phapi_lib.ProjectHostingApiClient(mox.IgnoreArg(),
                                                   mox.IgnoreArg())
        self.mox.ReplayAll()
        issue = self.mox.CreateMock(gdata_lib.Issue)
        issue.labels = labels

        reporter = reporting.Reporter()
        new_labels, count = reporter._create_autofiled_count_update(issue)
        expected = map(lambda l: '-' + l, remove)
        expected.append(self._create_count_label(expected_count))
        self.assertEqual(new_labels, expected)
        self.assertEqual(count, expected_count)


    def testProjectLabelExtraction(self):
        """Test that the project label is correctly extracted from the title."""
        TITLE_EMPTY = ''
        TITLE_NO_PROJ = '[stress] platformDevice Failure on release/47-75.0.0'
        TITLE_PROJ = '[stress] p_Device Failure on rikku-release/R44-7075.0.0'
        TITLE_PROJ2 = '[stress] p_Device Failure on ' \
                      'rikku-freon-release/R44-7075.0.0'
        TITLE_PROJ_SUBBOARD = '[stress] p_Device Failure on ' \
                              'veyron_rikku-release/R44-7075.0.0'

        client = phapi_lib.ProjectHostingApiClient(mox.IgnoreArg(),
                                                   mox.IgnoreArg())
        self.mox.ReplayAll()

        reporter = reporting.Reporter()
        self.assertEqual(reporter._get_project_label_from_title(TITLE_EMPTY),
                '')
        self.assertEqual(reporter._get_project_label_from_title(
                TITLE_NO_PROJ), '')
        self.assertEqual(reporter._get_project_label_from_title(TITLE_PROJ),
                'Proj-rikku')
        self.assertEqual(reporter._get_project_label_from_title(TITLE_PROJ2),
                'Proj-rikku')
        self.assertEqual(reporter._get_project_label_from_title(
                TITLE_PROJ_SUBBOARD), 'Proj-rikku')


    def testCountLabelIncrement(self):
        """Test that incrementing an autofiled-count label should work."""
        n = 3
        old_label = self._create_count_label(n)
        self._test_count_label_update([old_label], [old_label], n + 1)


    def testCountLabelIncrementPredefined(self):
        """Test that Reporter._PREDEFINED_LABELS has a sane autofiled-count."""
        self._test_count_label_update(
                reporting.Reporter._PREDEFINED_LABELS,
                [self._create_count_label(1)], 2)


    def testCountLabelCreate(self):
        """Test that old bugs should get a correct autofiled-count."""
        self._test_count_label_update([], [], 2)


    def testCountLabelIncrementMultiple(self):
        """Test that duplicate autofiled-count labels are handled."""
        old_count1 = self._create_count_label(2)
        old_count2 = self._create_count_label(3)
        self._test_count_label_update([old_count1, old_count2],
                                      [old_count1, old_count2], 4)


    def testCountLabelSkipUnknown(self):
        """Test that autofiled-count increment ignores unknown labels."""
        old_count = self._create_count_label(3)
        self._test_count_label_update(['unknown-label', old_count],
                                      [old_count], 4)


    def testCountLabelSkipMalformed(self):
        """Test that autofiled-count increment ignores unusual labels."""
        old_count = self._create_count_label(3)
        self._test_count_label_update(
                [reporting.Reporter.AUTOFILED_COUNT + 'bogus',
                 self._create_count_label(8) + '-bogus',
                 old_count],
                [old_count], 4)


class TestSubmitGenericBugReport(mox.MoxTestBase, unittest.TestCase):
    """Test the submit_generic_bug_report function."""

    def setUp(self):
        super(TestSubmitGenericBugReport, self).setUp()
        self.mox.StubOutClassWithMocks(reporting, 'Reporter')


    def test_accepts_required_arguments(self):
        """
        Test that the function accepts the required arguments.

        This basically tests that no exceptions are thrown.

        """
        reporter = reporting.Reporter()
        reporter.report(mox.IgnoreArg()).AndReturn((11,1))

        self.mox.ReplayAll()
        reporting.submit_generic_bug_report('title', 'summary')


    def test_rejects_too_few_required_arguments(self):
        """Test that the function rejects too few required arguments."""
        self.mox.ReplayAll()
        self.assertRaises(TypeError,
                          reporting.submit_generic_bug_report, 'too_few')


    def test_accepts_key_word_arguments(self):
        """
        Test that the functions accepts the key_word arguments.

        This basically tests that no exceptions are thrown.

        """
        reporter = reporting.Reporter()
        reporter.report(mox.IgnoreArg()).AndReturn((11,1))

        self.mox.ReplayAll()
        reporting.submit_generic_bug_report('test', 'summary', labels=[])


    def test_rejects_invalid_keyword_arguments(self):
        """Test that the function rejects invalid keyword arguments."""
        self.mox.ReplayAll()
        self.assertRaises(TypeError, reporting.submit_generic_bug_report,
                          'title', 'summary', wrong='wrong')


class TestMergeBugTemplate(mox.MoxTestBase):
    """Test bug can be properly merged and validated."""
    def test_validate_success(self):
        """Test a valid bug can be verified successfully."""
        bug_template= {}
        bug_template['owner'] = 'someone@company.com'
        reporting_utils.BugTemplate.validate_bug_template(bug_template)


    def test_validate_success(self):
        """Test a valid bug can be verified successfully."""
        # Bug template must be a dictionary.
        bug_template = ['test']
        self.assertRaises(reporting_utils.InvalidBugTemplateException,
                          reporting_utils.BugTemplate.validate_bug_template,
                          bug_template)

        # Bug template must contain value for essential attribute, e.g., owner.
        bug_template= {'no-owner': 'user1'}
        self.assertRaises(reporting_utils.InvalidBugTemplateException,
                          reporting_utils.BugTemplate.validate_bug_template,
                          bug_template)

        # Bug template must contain value for essential attribute, e.g., owner.
        bug_template= {'owner': 'invalid_email_address'}
        self.assertRaises(reporting_utils.InvalidBugTemplateException,
                          reporting_utils.BugTemplate.validate_bug_template,
                          bug_template)

        # Check unexpected attributes.
        bug_template= {}
        bug_template['random tag'] = 'test'
        self.assertRaises(reporting_utils.InvalidBugTemplateException,
                          reporting_utils.BugTemplate.validate_bug_template,
                          bug_template)

        # Value for cc must be a list
        bug_template= {}
        bug_template['cc'] = 'test'
        self.assertRaises(reporting_utils.InvalidBugTemplateException,
                          reporting_utils.BugTemplate.validate_bug_template,
                          bug_template)

        # Value for labels must be a list
        bug_template= {}
        bug_template['labels'] = 'test'
        self.assertRaises(reporting_utils.InvalidBugTemplateException,
                          reporting_utils.BugTemplate.validate_bug_template,
                          bug_template)


    def test_merge_success(self):
        """Test test and suite bug templates can be merged successfully."""
        test_bug_template = {
            'labels': ['l1'],
            'owner': 'user1@chromium.org',
            'status': 'Assigned',
            'title': None,
            'cc': ['cc1@chromium.org', 'cc2@chromium.org']
        }
        suite_bug_template = {
            'labels': ['l2'],
            'owner': 'user2@chromium.org',
            'status': 'Fixed',
            'summary': 'This is a short summary for suite bug',
            'title': 'Title for suite bug',
            'cc': ['cc2@chromium.org', 'cc3@chromium.org']
        }
        bug_template = reporting_utils.BugTemplate(suite_bug_template)
        merged_bug_template = bug_template.finalize_bug_template(
                test_bug_template)
        self.assertEqual(merged_bug_template['owner'],
                         test_bug_template['owner'],
                         'Value in test bug template should prevail.')

        self.assertEqual(merged_bug_template['title'],
                         suite_bug_template['title'],
                         'If an attribute has value None in test bug template, '
                         'use the value given in suite bug template.')

        self.assertEqual(merged_bug_template['summary'],
                         suite_bug_template['summary'],
                         'If an attribute does not exist in test bug template, '
                         'but exists in suite bug template, it should be '
                         'included in the merged template.')

        self.assertEqual(merged_bug_template['cc'],
                         test_bug_template['cc'] + suite_bug_template['cc'],
                         'List values for an attribute should be merged.')

        self.assertEqual(merged_bug_template['labels'],
                         test_bug_template['labels'] +
                         suite_bug_template['labels'],
                         'List values for an attribute should be merged.')

        test_bug_template['owner'] = ''
        test_bug_template['cc'] = ['']
        suite_bug_template['owner'] = ''
        suite_bug_template['cc'] = ['']
        bug_template = reporting_utils.BugTemplate(suite_bug_template)
        merged_bug_template = bug_template.finalize_bug_template(
                test_bug_template)
        self.assertFalse('owner' in merged_bug_template,
                         'owner should be removed from the merged template.')
        self.assertFalse('cc' in merged_bug_template,
                         'cc should be removed from the merged template.')


if __name__ == '__main__':
    unittest.main()
