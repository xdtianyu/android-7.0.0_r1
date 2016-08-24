#!/usr/bin/python
#
# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import datetime, unittest

import mox

import common

# We want to import setup_django_lite_environment first so that the database
# is setup correctly.
from autotest_lib.frontend import setup_django_lite_environment
from autotest_lib.client.common_lib import utils
from autotest_lib.frontend.afe import models, rpc_interface
from django import test
from autotest_lib.server.cros import repair_utils


# See complete_failures_functional_tests.py for why we need this.
class MockDatetime(datetime.datetime):
    """Used to mock out parts of datetime.datetime."""
    pass


# We use a mock rpc client object so that we instead directly use the server.
class MockAFE():
    """Used to mock out the rpc client."""
    def run(self, func, **kwargs):
        """
        The fake run call directly contacts the server.

        @param func: The name of the remote function that is being called.

        @param kwargs: The arguments to the remotely called function.
        """
        return utils.strip_unicode(getattr(rpc_interface, func)(**kwargs))


class FindProblemTestTests(mox.MoxTestBase, test.TestCase):
    """Test that we properly find the last ran job."""


    def setUp(self):
        super(FindProblemTestTests, self).setUp()
        self.mox.StubOutWithMock(MockDatetime, 'today')

        self.datetime = datetime.datetime
        datetime.datetime = MockDatetime
        self._orig_cutoff = repair_utils._CUTOFF_AFTER_TIMEOUT_MINS
        self._orig_timeout = repair_utils._DEFAULT_TEST_TIMEOUT_MINS


    def tearDown(self):
        repair_utils._DEFAULT_TEST_TIMEOUT_MINS = self._orig_timeout
        repair_utils._CUTOFF_AFTER_TIMEOUT_MINS = self._orig_cutoff
        datetime.datetime = self.datetime
        super(FindProblemTestTests, self).tearDown()


    def test_should_get_most_recent_job(self):
        """Test that, for a given host, we get the last job ran on that host."""

        host = models.Host(hostname='host')
        host.save()

        old_job = models.Job(owner='me', name='old_job',
                             created_on=datetime.datetime(2012, 1, 1))
        old_job.save()
        old_host_queue_entry = models.HostQueueEntry(
                job=old_job, host=host, status='test',
                started_on=datetime.datetime(2012, 1, 1, 1))
        old_host_queue_entry.save()

        new_job = models.Job(owner='me', name='new_job',
                             created_on=datetime.datetime(2012, 1, 1))
        new_job.save()
        new_host_queue_entry = models.HostQueueEntry(
                job=new_job, host=host, status='test',
                started_on=datetime.datetime(2012, 1, 1, 2))
        new_host_queue_entry.save()

        mock_rpc = MockAFE()
        datetime.datetime.today().AndReturn(datetime.datetime(2012,1,1))
        repair_utils._DEFAULT_TEST_TIMEOUT_MINS = 1440

        repair_utils._CUTOFF_AFTER_TIMEOUT_MINS = 60

        self.mox.ReplayAll()
        result = repair_utils._find_problem_test('host', mock_rpc)

        self.assertEqual(result['job']['name'], 'new_job')


    def test_should_get_job_for_specified_host_only(self):
        """Test that we only get a job that is for the given host."""

        correct_job = models.Job(owner='me', name='correct_job',
                                 created_on=datetime.datetime(2012, 1, 1))
        correct_job.save()
        correct_host = models.Host(hostname='correct_host')
        correct_host.save()
        correct_host_queue_entry = models.HostQueueEntry(
            job=correct_job, host=correct_host, status='test',
            started_on=datetime.datetime(2012, 1, 1, 1))
        correct_host_queue_entry.save()

        wrong_job = models.Job(owner='me', name='wrong_job',
                               created_on=datetime.datetime(2012, 1, 1))
        wrong_job.save()
        wrong_host = models.Host(hostname='wrong_host')
        wrong_host.save()
        wrong_host_queue_entry = models.HostQueueEntry(
                job=wrong_job, host=wrong_host, status='test',
                started_on=datetime.datetime(2012, 1, 1, 2))
        wrong_host_queue_entry.save()

        mock_rpc = MockAFE()
        datetime.datetime.today().AndReturn(datetime.datetime(2012,1,1))
        repair_utils._DEFAULT_TEST_TIMEOUT_MINS = 1440

        repair_utils._CUTOFF_AFTER_TIMEOUT_MINS = 60

        self.mox.ReplayAll()
        result = repair_utils._find_problem_test('correct_host', mock_rpc)

        self.assertEqual(result['job']['name'], 'correct_job')


    def test_return_jobs_ran_soon_after_max_job_runtime(self):
        """Test that we get jobs that are just past the max runtime."""

        host = models.Host(hostname='host')
        host.save()

        new_job = models.Job(owner='me', name='new_job',
                             created_on=datetime.datetime(2012, 1, 1, 0, 0))
        new_job.save()
        new_host_queue_entry = models.HostQueueEntry(
                job=new_job, host=host, status='test',
                started_on=datetime.datetime(2012, 1, 1, 2))
        new_host_queue_entry.save()

        mock_rpc = MockAFE()
        datetime.datetime.today().AndReturn(datetime.datetime(2012, 1, 2, 0,
                                                              30))
        repair_utils._DEFAULT_TEST_TIMEOUT_MINS = 1440

        repair_utils._CUTOFF_AFTER_TIMEOUT_MINS = 60

        self.mox.ReplayAll()
        result = repair_utils._find_problem_test('host', mock_rpc)

        self.assertEqual(result['job']['name'], 'new_job')


if __name__ == '__main__':
    unittest.main()
