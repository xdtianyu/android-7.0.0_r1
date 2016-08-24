#!/usr/bin/python
#
# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import datetime, unittest

import mox

import common
# This must come before the import of utils in order to use the in memory
# database.
from autotest_lib.frontend import setup_django_readonly_environment
from autotest_lib.frontend import setup_test_environment
from autotest_lib.frontend.health import utils
from autotest_lib.frontend.tko import models
from django import test

ERROR_STATUS = models.Status(status_idx=2, word='ERROR')
ABORT_STATUS = models.Status(status_idx=3, word='ABORT')
FAIL_STATUS = models.Status(status_idx=4, word='FAIL')
WARN_STATUS = models.Status(status_idx=5, word='WARN')
GOOD_STATUS = models.Status(status_idx=6, word='GOOD')
ALERT_STATUS = models.Status(status_idx=7, word='ALERT')


def add_statuses():
    """
    Save the statuses to the in-memory database.

    These normally exist in the database and the code expects them. However, the
    normal test database setup does not do this for us.
    """
    ERROR_STATUS.save()
    ABORT_STATUS.save()
    FAIL_STATUS.save()
    WARN_STATUS.save()
    GOOD_STATUS.save()
    ALERT_STATUS.save()


class GetLastPassTimesTests(mox.MoxTestBase, test.TestCase):
    """Tests the get_last_pass_times function."""

    def setUp(self):
        super(GetLastPassTimesTests, self).setUp()
        setup_test_environment.set_up()
        add_statuses()


    def tearDown(self):
        setup_test_environment.tear_down()
        super(GetLastPassTimesTests, self).tearDown()


    def test_return_most_recent_pass(self):
        """The last time a test passed should be returned."""
        # To add a test entry to the database, the test object has to
        # be instantiated with various other model instances. We give these
        # instances dummy id values.
        job = models.Job(job_idx=1)
        kernel = models.Kernel(kernel_idx=1)
        machine = models.Machine(machine_idx=1)

        early_pass = models.Test(job=job, status=GOOD_STATUS,
                                 kernel=kernel, machine=machine,
                                 test='test',
                                 started_time=datetime.datetime(2012, 1, 1))
        early_pass.save()
        late_pass = models.Test(job=job, status=GOOD_STATUS,
                                kernel=kernel, machine=machine,
                                test='test',
                                started_time=datetime.datetime(2012, 1, 2))
        late_pass.save()

        results = utils.get_last_pass_times()

        self.assertEquals(results, {'test': datetime.datetime(2012, 1, 2)})


    def test_only_return_passing_tests(self):
        """Tests that only tests that have passed at some point are returned."""
        job = models.Job(job_idx=1)
        kernel = models.Kernel(kernel_idx=1)
        machine = models.Machine(machine_idx=1)

        passing_test = models.Test(job=job, status=GOOD_STATUS,
                                   kernel=kernel, machine=machine,
                                   test='passing_test',
                                   started_time=datetime.datetime(2012, 1, 1))
        passing_test.save()
        failing_test = models.Test(job=job, status=FAIL_STATUS,
                                   kernel=kernel, machine=machine,
                                   test='failing_test',
                                   started_time=datetime.datetime(2012, 1, 1))
        failing_test.save()

        results = utils.get_last_pass_times()

        self.assertEquals(results,
                          {'passing_test': datetime.datetime(2012, 1, 1)})


    def test_return_all_passing_tests(self):
        """This function returns all tests that passed at least once."""
        job = models.Job(job_idx=1)
        kernel = models.Kernel(kernel_idx=1)
        machine = models.Machine(machine_idx=1)

        test1 = models.Test(job=job, status=GOOD_STATUS,
                            kernel=kernel, machine=machine,
                            test='test1',
                            started_time=datetime.datetime(2012, 1, 1))
        test1.save()
        test2 = models.Test(job=job, status=GOOD_STATUS,
                            kernel=kernel, machine=machine,
                            test='test2',
                            started_time=datetime.datetime(2012, 1, 2))
        test2.save()

        results = utils.get_last_pass_times()

        self.assertEquals(results, {'test1': datetime.datetime(2012, 1, 1),
                                    'test2': datetime.datetime(2012, 1, 2)})


class GetLastFailTimesTests(mox.MoxTestBase, test.TestCase):
    """Tests the get_last_fail_times function."""

    def setUp(self):
        super(GetLastFailTimesTests, self).setUp()
        setup_test_environment.set_up()
        add_statuses()


    def tearDown(self):
        setup_test_environment.tear_down()
        super(GetLastFailTimesTests, self).tearDown()


    def test_return_most_recent_fail(self):
        """The last time a test failed should be returned."""
        # To add a test entry to the database, the test object has to
        # be instantiated with various other model instances. We give these
        # instances dummy id values.
        job = models.Job(job_idx=1)
        kernel = models.Kernel(kernel_idx=1)
        machine = models.Machine(machine_idx=1)

        early_fail = models.Test(job=job, status=FAIL_STATUS,
                                 kernel=kernel, machine=machine,
                                 test='test',
                                 started_time=datetime.datetime(2012, 1, 1))
        early_fail.save()
        late_fail = models.Test(job=job, status=FAIL_STATUS,
                                kernel=kernel, machine=machine,
                                test='test',
                                started_time=datetime.datetime(2012, 1, 2))
        late_fail.save()

        results = utils.get_last_fail_times()

        self.assertEquals(results, {'test': datetime.datetime(2012, 1, 2)})


    def test_does_not_return_passing_tests(self):
        """Test that passing test entries are not included."""
        job = models.Job(job_idx=1)
        kernel = models.Kernel(kernel_idx=1)
        machine = models.Machine(machine_idx=1)

        passing_test = models.Test(job=job, status=GOOD_STATUS,
                                   kernel=kernel, machine=machine,
                                   test='passing_test',
                                   started_time=datetime.datetime(2012, 1, 1))
        passing_test.save()
        failing_test = models.Test(job=job, status=FAIL_STATUS,
                                   kernel=kernel, machine=machine,
                                   test='failing_test',
                                   started_time=datetime.datetime(2012, 1, 1))
        failing_test.save()

        results = utils.get_last_fail_times()

        self.assertEquals(results,
                          {'failing_test': datetime.datetime(2012, 1, 1)})


    def test_return_all_failing_tests(self):
        """This function returns all tests that failed at least once."""
        job = models.Job(job_idx=1)
        kernel = models.Kernel(kernel_idx=1)
        machine = models.Machine(machine_idx=1)

        test1 = models.Test(job=job, status=FAIL_STATUS,
                            kernel=kernel, machine=machine,
                            test='test1',
                            started_time=datetime.datetime(2012, 1, 1))
        test1.save()
        test2 = models.Test(job=job, status=FAIL_STATUS,
                            kernel=kernel, machine=machine,
                            test='test2',
                            started_time=datetime.datetime(2012, 1, 2))
        test2.save()

        results = utils.get_last_fail_times()

        self.assertEquals(results, {'test1': datetime.datetime(2012, 1, 1),
                                    'test2': datetime.datetime(2012, 1, 2)})


    def test_returns_treats_error_status_as_failure(self):
        """Error statuses should count as a failure."""
        job = models.Job(job_idx=1)
        kernel = models.Kernel(kernel_idx=1)
        machine = models.Machine(machine_idx=1)

        test = models.Test(job=job, status=ERROR_STATUS,
                           kernel=kernel, machine=machine,
                           test='error',
                           started_time=datetime.datetime(2012, 1, 1))
        test.save()

        results = utils.get_last_fail_times()

        self.assertEquals(results, {'error': datetime.datetime(2012, 1, 1)})


    def test_returns_treats_abort_status_as_failure(self):
        """
        Abort statuses should count as failures.

        This should be changed once Abort only represents user caused aborts.
        See issue crbug.com/188217.
        """
        job = models.Job(job_idx=1)
        kernel = models.Kernel(kernel_idx=1)
        machine = models.Machine(machine_idx=1)

        test = models.Test(job=job, status=ABORT_STATUS,
                           kernel=kernel, machine=machine,
                           test='abort',
                           started_time=datetime.datetime(2012, 1, 1))
        test.save()

        results = utils.get_last_fail_times()

        self.assertEquals(results, {'abort': datetime.datetime(2012, 1, 1)})


    def test_returns_treats_warn_status_as_failure(self):
        """Warn statuses should count as failures."""
        job = models.Job(job_idx=1)
        kernel = models.Kernel(kernel_idx=1)
        machine = models.Machine(machine_idx=1)

        test = models.Test(job=job, status=WARN_STATUS,
                           kernel=kernel, machine=machine,
                           test='warn',
                           started_time=datetime.datetime(2012, 1, 1))
        test.save()

        results = utils.get_last_fail_times()

        self.assertEquals(results, {'warn': datetime.datetime(2012, 1, 1)})


    def test_returns_treats_alert_status_as_failure(self):
        """Alert statuses should count as failures."""
        job = models.Job(job_idx=1)
        kernel = models.Kernel(kernel_idx=1)
        machine = models.Machine(machine_idx=1)

        test = models.Test(job=job, status=ALERT_STATUS,
                           kernel=kernel, machine=machine,
                           test='alert',
                           started_time=datetime.datetime(2012, 1, 1))
        test.save()

        results = utils.get_last_fail_times()

        self.assertEquals(results, {'alert': datetime.datetime(2012, 1, 1)})


if __name__ == '__main__':
    unittest.main()
