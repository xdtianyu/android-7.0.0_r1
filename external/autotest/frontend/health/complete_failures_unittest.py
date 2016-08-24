#!/usr/bin/python
#
# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import datetime, unittest

import mox

import common
# This must come before the import of complete_failures in order to use the
# in memory database.
from autotest_lib.frontend import setup_django_readonly_environment
from autotest_lib.frontend import setup_test_environment
import complete_failures
from autotest_lib.client.common_lib import mail
from autotest_lib.frontend.tko import models
from django import test


GOOD_STATUS_IDX = 6

# See complte_failurs_functional_tests.py for why we need this.
class MockDatetime(datetime.datetime):
    """Used to mock out parts of datetime.datetime."""
    pass


class EmailAboutTestFailureTests(mox.MoxTestBase):
    """
    Tests that emails are sent about failed tests.
    """
    def setUp(self):
        super(EmailAboutTestFailureTests, self).setUp()

        # We need to mock out the send function in all tests or else the
        # emails will be sent out during tests.
        self.mox.StubOutWithMock(mail, 'send')

        self._orig_too_long = complete_failures._DAYS_TO_BE_FAILING_TOO_LONG


    def tearDown(self):
        complete_failures._DAYS_TO_BE_FAILING_TOO_LONG = self._orig_too_long
        super(EmailAboutTestFailureTests, self).tearDown()


    def test_email_sent_about_all_failed_tests(self):
        """Test that the email report mentions all the failed_tests."""
        complete_failures._DAYS_TO_BE_FAILING_TOO_LONG = 60

        mail.send(
                'chromeos-test-health@google.com',
                ['chromeos-lab-infrastructure@google.com'],
                [],
                'Long Failing Tests',
                '1/1 tests have been failing for at least %d days.\n'
                'They are the following:\n\ntest'
                    % complete_failures._DAYS_TO_BE_FAILING_TOO_LONG)

        failures = ['test']
        all_tests = set(failures)

        self.mox.ReplayAll()
        complete_failures.email_about_test_failure(failures, all_tests)


    def test_email_has_test_names_sorted_alphabetically(self):
        """Test that the email report has entries sorted alphabetically."""
        complete_failures._DAYS_TO_BE_FAILING_TOO_LONG = 60

        mail.send(
                'chromeos-test-health@google.com',
                ['chromeos-lab-infrastructure@google.com'],
                [],
                'Long Failing Tests',
                '2/2 tests have been failing for at least %d days.\n'
                'They are the following:\n\ntest1\ntest2'
                    % complete_failures._DAYS_TO_BE_FAILING_TOO_LONG)

        # We use an OrderedDict to gurantee that the elements would be out of
        # order if we did a simple traversal.
        failures = ['test2', 'test1']
        all_tests = set(failures)

        self.mox.ReplayAll()
        complete_failures.email_about_test_failure(failures, all_tests)


    def test_email_count_of_total_number_of_tests(self):
        """Test that the email report displays total number of tests."""
        complete_failures._DAYS_TO_BE_FAILING_TOO_LONG = 60

        mail.send(
                'chromeos-test-health@google.com',
                ['chromeos-lab-infrastructure@google.com'],
                [],
                'Long Failing Tests',
                '1/2 tests have been failing for at least %d days.\n'
                'They are the following:\n\ntest'
                    % complete_failures._DAYS_TO_BE_FAILING_TOO_LONG)

        failures = ['test']
        all_tests = set(failures) | {'not_failure'}

        self.mox.ReplayAll()
        complete_failures.email_about_test_failure(failures, all_tests)


class IsValidTestNameTests(test.TestCase):
    """Tests the is_valid_test_name function."""

    def test_returns_true_for_valid_test_name(self):
        """Test that a valid test name returns True."""
        name = 'TestName.TestName'
        self.assertTrue(complete_failures.is_valid_test_name(name))


    def test_returns_false_if_name_has_slash_in_it(self):
        """Test that a name with a slash in it returns False."""
        name = 'path/to/test'
        self.assertFalse(complete_failures.is_valid_test_name(name))


    def test_returns_false_for_try_new_image_entries(self):
        """Test that a name that starts with try_new_image returns False."""
        name = 'try_new_image-blah'
        self.assertFalse(complete_failures.is_valid_test_name(name))


class PrepareLastPassesTests(test.TestCase):
    """Tests the prepare_last_passes function."""

    def setUp(self):
        super(PrepareLastPassesTests, self).setUp()

    def tearDown(self):
        super(PrepareLastPassesTests, self).tearDown()

    def test_does_not_return_invalid_test_names(self):
        """Tests that tests with invalid test names are not returned."""
        results = complete_failures.prepare_last_passes(['invalid_test/name'])

        self.assertEqual(results, {})


class GetRecentlyRanTestNamesTests(mox.MoxTestBase, test.TestCase):
    """Tests the get_recently_ran_test_names function."""

    def setUp(self):
        super(GetRecentlyRanTestNamesTests, self).setUp()
        self.mox.StubOutWithMock(MockDatetime, 'today')
        self.datetime = datetime.datetime
        datetime.datetime = MockDatetime
        setup_test_environment.set_up()
        self._orig_cutoff = complete_failures._DAYS_NOT_RUNNING_CUTOFF


    def tearDown(self):
        datetime.datetime = self.datetime
        complete_failures._DAYS_NOT_RUNNING_CUTOFF = self._orig_cutoff
        setup_test_environment.tear_down()
        super(GetRecentlyRanTestNamesTests, self).tearDown()


    def test_return_all_recently_ran_tests(self):
        """Test that the function does as it says it does."""
        job = models.Job(job_idx=1)
        kernel = models.Kernel(kernel_idx=1)
        machine = models.Machine(machine_idx=1)
        success_status = models.Status(status_idx=GOOD_STATUS_IDX)

        recent = models.Test(job=job, status=success_status,
                             kernel=kernel, machine=machine,
                             test='recent',
                             started_time=self.datetime(2012, 1, 1))
        recent.save()
        old = models.Test(job=job, status=success_status,
                          kernel=kernel, machine=machine,
                          test='old',
                          started_time=self.datetime(2011, 1, 2))
        old.save()

        datetime.datetime.today().AndReturn(self.datetime(2012, 1, 4))
        complete_failures._DAYS_NOT_RUNNING_CUTOFF = 60

        self.mox.ReplayAll()
        results = complete_failures.get_recently_ran_test_names()

        self.assertEqual(set(results), set(['recent']))


    def test_returns_no_duplicate_names(self):
        """Test that each test name appears only once."""
        job = models.Job(job_idx=1)
        kernel = models.Kernel(kernel_idx=1)
        machine = models.Machine(machine_idx=1)
        success_status = models.Status(status_idx=GOOD_STATUS_IDX)

        test = models.Test(job=job, status=success_status,
                           kernel=kernel, machine=machine,
                           test='test',
                           started_time=self.datetime(2012, 1, 1))
        test.save()
        duplicate = models.Test(job=job, status=success_status,
                                kernel=kernel, machine=machine,
                                test='test',
                                started_time=self.datetime(2012, 1, 2))
        duplicate.save()

        datetime.datetime.today().AndReturn(self.datetime(2012, 1, 3))
        complete_failures._DAYS_NOT_RUNNING_CUTOFF = 60

        self.mox.ReplayAll()
        results = complete_failures.get_recently_ran_test_names()

        self.assertEqual(len(results), 1)


class GetTestsToAnalyzeTests(mox.MoxTestBase):
    """Tests the get_tests_to_analyze function."""

    def test_returns_recent_test_names(self):
        """Test should return all the test names in the database."""

        recent_tests = {'passing_test', 'failing_test'}
        last_passes = {'passing_test': datetime.datetime(2012, 1 ,1),
                       'old_passing_test': datetime.datetime(2011, 1, 1)}

        results = complete_failures.get_tests_to_analyze(recent_tests,
                                                         last_passes)

        self.assertEqual(results,
                         {'passing_test': datetime.datetime(2012, 1, 1),
                          'failing_test': datetime.datetime.min})


    def test_returns_failing_tests_with_min_datetime(self):
        """Test that never-passed tests are paired with datetime.min."""

        recent_tests = {'test'}
        last_passes = {}

        self.mox.ReplayAll()
        results = complete_failures.get_tests_to_analyze(recent_tests,
                                                         last_passes)

        self.assertEqual(results, {'test': datetime.datetime.min})


class FilterOutGoodTestsTests(mox.MoxTestBase):
    """Tests the filter_our_good_tests function."""

    def setUp(self):
        super(FilterOutGoodTestsTests, self).setUp()
        self.mox.StubOutWithMock(MockDatetime, 'today')
        self.datetime = datetime.datetime
        datetime.datetime = MockDatetime
        self._orig_too_long = complete_failures._DAYS_TO_BE_FAILING_TOO_LONG


    def tearDown(self):
        datetime.datetime = self.datetime
        complete_failures._DAYS_TO_BE_FAILING_TOO_LONG = self._orig_too_long
        super(FilterOutGoodTestsTests, self).tearDown()


    def test_remove_all_tests_that_have_passed_recently_enough(self):
        """Test that all recently passing tests are not returned."""

        complete_failures._DAYS_TO_BE_FAILING_TOO_LONG = 10
        datetime.datetime.today().AndReturn(self.datetime(2012, 1, 21))

        self.mox.ReplayAll()
        result = complete_failures.filter_out_good_tests(
                {'test': self.datetime(2012, 1, 20)})

        self.assertEqual(result, [])


    def test_keep_all_tests_that_have_not_passed_recently_enough(self):
        """Test that the tests that have not recently passed are returned."""

        complete_failures._DAYS_TO_BE_FAILING_TOO_LONG = 10
        datetime.datetime.today().AndReturn(self.datetime(2012, 1, 21))

        self.mox.ReplayAll()
        result = complete_failures.filter_out_good_tests(
                {'test': self.datetime(2012, 1, 10)})

        self.assertEqual(result, ['test'])


if __name__ == '__main__':
    unittest.main()
