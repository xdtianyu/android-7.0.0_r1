#! /usr/bin/python

# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from datetime import timedelta, datetime
import mock
import unittest

import common
from autotest_lib.frontend import setup_django_readonly_environment
from autotest_lib.frontend import setup_test_environment
from autotest_lib.frontend.afe import models
from autotest_lib.site_utils import count_jobs
from django import test


class TestCountJobs(test.TestCase):
    """Tests the count_jobs script's functionality.
    """

    def setUp(self):
        super(TestCountJobs, self).setUp()
        setup_test_environment.set_up()


    def tearDown(self):
        super(TestCountJobs, self).tearDown()
        setup_test_environment.tear_down()


    def test_no_jobs(self):
        """Initially, there should be no jobs."""
        self.assertEqual(
            0,
            count_jobs.number_of_jobs_since(timedelta(days=999)))


    def test_count_jobs(self):
        """When n jobs are inserted, n jobs should be counted within a day range.

        Furthermore, 0 jobs should be counted within 0 or (-1) days.
        """
        some_day = datetime.fromtimestamp(1450211914)  # a time grabbed from time.time()
        class FakeDatetime(datetime):
            """Always returns the same 'now' value"""
            @classmethod
            def now(self):
                """Return a fake 'now', rather than rely on the system's clock."""
                return some_day
        with mock.patch.object(count_jobs, 'datetime', FakeDatetime):
            for i in range(1, 24):
                 models.Job(created_on=some_day - timedelta(hours=i)).save()
                 for count, days in ((i, 1), (0, 0), (0, -1)):
                     self.assertEqual(
                        count,
                        count_jobs.number_of_jobs_since(timedelta(days=days)))


if __name__ == '__main__':
    unittest.main()
