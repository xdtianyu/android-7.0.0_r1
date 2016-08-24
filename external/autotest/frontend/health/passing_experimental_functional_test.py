#!/usr/bin/python
#
# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import datetime, subprocess, unittest

import mox

import common
# This must come before the import of complete_failures in order to use the
# in-memory database.
from autotest_lib.frontend import setup_django_readonly_environment
from autotest_lib.frontend import setup_test_environment
from autotest_lib.frontend.health import passing_experimental
from autotest_lib.frontend.afe import models as afe_models
from autotest_lib.frontend.tko import models as tko_models
from autotest_lib.server.cros.dynamic_suite import reporting
from django import test


GOOD_STATUS_IDX = 6
FAIL_STATUS_IDX = 4

# During the tests there is a point where Django does a type check on
# datetime.datetime. Unfortunately this means when datetime is mocked out,
# horrible failures happen when Django tries to do this check. The solution
# chosen is to create a pure Python class that inheirits from datetime.datetime
# so that the today class method can be directly mocked out. It is necesarry
# to mock out datetime.datetime completely as it a C class and so cannot have
# parts of itself mocked out.
class MockDatetime(datetime.datetime):
    """Used to mock out parts of datetime.datetime."""
    pass


class PassingExperimentalFunctionalTests(mox.MoxTestBase, test.TestCase):
    """
    Does a functional test of the passing_experimental.py script.

    It uses an in-memory database, mocks out the saving and loading of the
    storage object and mocks out the sending of the bugs. Everything else
    is a full run.

    """

    def setUp(self):
        super(PassingExperimentalFunctionalTests, self).setUp()
        setup_test_environment.set_up()
        # All of our tests will involve mocking out the datetime.today() class
        # method.
        self.mox.StubOutWithMock(MockDatetime, 'today')
        self.datetime = datetime.datetime
        datetime.datetime = MockDatetime
        # We really do not want a script that modifies the DB to run during
        # testing. So we will mock this out even though we will mock out the
        # function that calls it in case of refactoring.
        self.mox.StubOutWithMock(subprocess, 'call')
        # We need to mock out this function so bugs are not filed.
        self.mox.StubOutClassWithMocks(reporting, 'Bug')
        self.mox.StubOutClassWithMocks(reporting, 'Reporter')
        self._orig_since_failure = passing_experimental._MIN_DAYS_SINCE_FAILURE
        self._orig_since_pass = passing_experimental._MAX_DAYS_SINCE_LAST_PASS


    def tearDown(self):
        passing_experimental._MAX_DAYS_SINCE_LAST_PASS = self._orig_since_pass
        passing_experimental._MIN_DAYS_SINCE_FAILURE = self._orig_since_failure
        datetime.datetime = self.datetime
        setup_test_environment.tear_down()
        super(PassingExperimentalFunctionalTests, self).tearDown()


    def test(self):
        """Does a basic test of as much of the system as possible."""
        afe_models.Test(name='test1', test_type=0, path='test1',
            experimental=True).save()
        afe_models.Test(name='test2', test_type=0, path='test2',
            experimental=True).save()

        tko_models.Status(status_idx=6, word='GOOD').save()

        job = tko_models.Job(job_idx=1)
        kernel = tko_models.Kernel(kernel_idx=1)
        machine = tko_models.Machine(machine_idx=1)
        success_status = tko_models.Status(status_idx=GOOD_STATUS_IDX)
        fail_status = tko_models.Status(status_idx=FAIL_STATUS_IDX)

        tko_test1 = tko_models.Test(job=job, status=success_status,
                                    kernel=kernel, machine=machine,
                                    test='test1',
                                    started_time=self.datetime(2012, 1, 20))
        tko_test1.save()
        tko_test2 = tko_models.Test(job=job, status=success_status,
                                    kernel=kernel, machine=machine,
                                    test='test2',
                                    started_time=self.datetime(2012, 1, 20))
        tko_test2.save()

        passing_experimental._MAX_DAYS_SINCE_LAST_PASS = 10
        passing_experimental._MIN_DAYS_SINCE_FAILURE = 10

        MockDatetime.today().AndReturn(self.datetime(2012, 1, 21))
        MockDatetime.today().AndReturn(self.datetime(2012, 1, 21))
        reporter1 = reporting.Reporter()
        bug1 = reporting.Bug(
                title=u'test1 should be promoted to non-experimental.',
                summary=mox.IgnoreArg(),
                search_marker=u'PassingExperimental(test1)')
        reporter1.report(bug1).AndReturn((11, 1))
        reporter2 = reporting.Reporter()
        bug2 = reporting.Bug(
                title=u'test2 should be promoted to non-experimental.',
                summary=mox.IgnoreArg(),
                search_marker=u'PassingExperimental(test2)')
        reporter2.report(bug2).AndReturn((11, 1))

        self.mox.ReplayAll()
        passing_experimental.main()


if __name__ == '__main__':
    unittest.main()
