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
from autotest_lib.frontend.health import passing_experimental
from autotest_lib.frontend.afe import models as afe_models
from django import test


# datetime.datetime is all C code and so cannot be mocked out in a normal
# fashion.
class MockDatetime(datetime.datetime):
    """Used to mock out parts of datetime.datetime."""
    pass


class GetExperimentalTestsTests(test.TestCase):
    """Tests the get_experimetnal_tests function."""

    def setUp(self):
        super(GetExperimentalTestsTests, self).setUp()
        setup_test_environment.set_up()


    def tearDown(self):
        setup_test_environment.tear_down()
        super(GetExperimentalTestsTests, self).tearDown()


    def test_returns_tests_marked_experimental(self):
        """Test that tests marked as experimental are returned."""
        test = afe_models.Test(name='test', test_type=0,
                               experimental=True)
        test.save()

        result = passing_experimental.get_experimental_tests()

        self.assertEqual(result, set(['test']))


    def test_does_not_return_tests_not_marked_experimental(self):
        """Test that tests not marked as experimetnal are not returned."""
        test = afe_models.Test(name='test', test_type=0,
                               experimental=False)
        test.save()

        result = passing_experimental.get_experimental_tests()

        self.assertEqual(result, set())


class FindLongPassingTestsTests(mox.MoxTestBase, test.TestCase):
    """Tests the find_long_passing_tests function."""
    def setUp(self):
        super(FindLongPassingTestsTests, self).setUp()
        self.mox.StubOutWithMock(MockDatetime, 'today')
        self._datetime = datetime.datetime
        datetime.datetime = MockDatetime
        self._orig_since_failure = passing_experimental._MIN_DAYS_SINCE_FAILURE
        self._orig_since_pass = passing_experimental._MAX_DAYS_SINCE_LAST_PASS


    def tearDown(self):
        passing_experimental._MAX_DAYS_SINCE_LAST_PASS = self._orig_since_pass
        passing_experimental._MIN_DAYS_SINCE_FAILURE = self._orig_since_failure
        datetime.datetime = self._datetime
        super(FindLongPassingTestsTests, self).tearDown()


    def test_do_not_return_tests_that_have_failed_recently(self):
        """Test that tests that have failed recently are not returned."""
        passing_experimental._MIN_DAYS_SINCE_FAILURE = 10
        datetime.datetime.today().AndReturn(self._datetime(2013, 3, 20))
        datetime.datetime.today().AndReturn(self._datetime(2013, 3, 20))

        pass_times = {'test': self._datetime(2013, 3, 12)}
        fail_times = {'test': self._datetime(2013, 3, 13)}
        valid_tests = {'test'}

        self.mox.ReplayAll()
        results = passing_experimental.find_long_passing_tests(pass_times,
                                                               fail_times,
                                                               valid_tests)

        self.assertEqual(results, set([]))


    def test_return_tests_that_have_recent_pass_but_not_recent_failure(self):
        """Test returning tests that have recently passed but not failed."""
        passing_experimental._MIN_DAYS_SINCE_FAILURE = 10
        passing_experimental._MAX_DAYS_SINCE_LAST_PASS = 10
        datetime.datetime.today().AndReturn(self._datetime(2013, 3, 20))
        datetime.datetime.today().AndReturn(self._datetime(2013, 3, 20))

        pass_times = {'test': self._datetime(2013, 3, 12)}
        fail_times = {'test': self._datetime(2013, 3, 1)}
        valid_tests = {'test'}

        self.mox.ReplayAll()
        results = passing_experimental.find_long_passing_tests(pass_times,
                                                               fail_times,
                                                               valid_tests)

        self.assertEqual(results, set(['test']))


    def test_filter_out_tests_that_have_not_passed_recently(self):
        """Test that tests that have not recently passed are not returned."""
        passing_experimental._MIN_DAYS_SINCE_FAILURE = 10
        passing_experimental._MAX_DAYS_SINCE_LAST_PASS = 10
        datetime.datetime.today().AndReturn(self._datetime(2013, 3, 20))
        datetime.datetime.today().AndReturn(self._datetime(2013, 3, 20))

        pass_times = {'test': self._datetime(2013, 3, 1)}
        fail_times = {'test': self._datetime(2013, 3, 1)}
        valid_tests = {'test'}

        self.mox.ReplayAll()
        results = passing_experimental.find_long_passing_tests(pass_times,
                                                               fail_times,
                                                               valid_tests)

        self.assertEqual(results, set([]))


    def test_filter_out_tests_that_are_not_valid(self):
        """Test that tests that are not valid are not returned."""
        passing_experimental._MIN_DAYS_SINCE_FAILURE = 10
        passing_experimental._MAX_DAYS_SINCE_LAST_PASS = 10
        datetime.datetime.today().AndReturn(self._datetime(2013, 3, 20))
        datetime.datetime.today().AndReturn(self._datetime(2013, 3, 20))

        pass_times = {'test2': self._datetime(2013, 3, 1)}
        fail_times = {'test2': self._datetime(2013, 3, 1)}
        valid_tests = {'test'}

        self.mox.ReplayAll()
        results = passing_experimental.find_long_passing_tests(pass_times,
                                                               fail_times,
                                                               valid_tests)

        self.assertEqual(results, set([]))


    def test_return_tests_that_have_recently_passed_and_never_failed(self):
        """Test that we can handle tests that have never failed."""
        passing_experimental._MIN_DAYS_SINCE_FAILURE = 10
        passing_experimental._MAX_DAYS_SINCE_LAST_PASS = 10
        datetime.datetime.today().AndReturn(self._datetime(2013, 3, 20))
        datetime.datetime.today().AndReturn(self._datetime(2013, 3, 20))

        pass_times = {'test': self._datetime(2013, 3, 11)}
        fail_times = {}
        valid_tests = {'test'}

        self.mox.ReplayAll()
        results = passing_experimental.find_long_passing_tests(pass_times,
                                                               fail_times,
                                                               valid_tests)

        self.assertEqual(results, set(['test']))


    def test_handle_tests_that_have_never_passed(self):
        """Test that we can handle tests that have never passed."""
        passing_experimental._MIN_DAYS_SINCE_FAILURE = 10
        passing_experimental._MAX_DAYS_SINCE_LAST_PASS = 10
        datetime.datetime.today().AndReturn(self._datetime(2013, 3, 20))
        datetime.datetime.today().AndReturn(self._datetime(2013, 3, 20))

        pass_times = {}
        fail_times = {'test': self._datetime(2013, 3, 11)}
        valid_tests = {'test'}

        self.mox.ReplayAll()
        results = passing_experimental.find_long_passing_tests(pass_times,
                                                               fail_times,
                                                               valid_tests)

        self.assertEqual(results, set([]))


if __name__ == '__main__':
    unittest.main()
