# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import random, time

from autotest_lib.client.bin import test
from autotest_lib.client.cros import sys_power


# Suspend tests need to allow time for the kernel to settle.
MIN_ALLOWED_SUSPEND_S = 10


class power_CheckAfterSuspend(test.test):
    """Checks capabilities by running tests after suspend/resume cycle.

    This can easily run multiple iterations with the built in Autotest
    iterations parameter to run_test():

      test_that -b <board> --iterations=3 ${MACHINE_IP} \
          f:client/site_tests/power_CheckAfterSuspend/control
    """
    version = 1


    def initialize(self, tests=[], min_suspend_s=2, enable_baseline=False):
        """
        @param tests: list of client tests to run before/after suspend.
        @param min_suspend_s: suspend durations (in seconds).
        @param enable_baseline: If True, run one pass of tests before suspend,
                                otherwise only run tests after suspend.
        """
        self._tests = tests
        self._min_suspend_s = min_suspend_s
        self._enable_baseline = enable_baseline


    def run_once(self):
        """Run a series of tests supplied by the control file.

        Handles iterations by adding tags with the iteration#.

        Normally runs each test once after each suspend.  If enable_baseline
        is True then run an initial pass through the tests before any suspend.

        The test runs a series
        """
        if self.iteration is not None and self.iteration > 1:
            test_tag = '%03d' % self.iteration
        else:
            test_tag = ''
            if self._enable_baseline:
                for t in self._tests:
                    self.job.run_test(t, tag=test_tag+'preSuspend', disable_sysinfo=True)

        time.sleep(random.randint(0, 3))
        sys_power.do_suspend(max(self._min_suspend_s, MIN_ALLOWED_SUSPEND_S))

        for t in self._tests:
            self.job.run_test(t, tag=test_tag+'postSuspend', disable_sysinfo=True)
