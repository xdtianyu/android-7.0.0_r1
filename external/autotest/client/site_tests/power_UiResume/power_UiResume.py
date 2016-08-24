# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import time

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib.cros import chrome
from autotest_lib.client.cros import power_suspend, sys_power


class power_UiResume(test.test):
    """
    Suspend a logged in system by sending a request to power manager via dbus.

    This test waits quite a bit after logging in and before suspending.
    Therefore it verifies suspend/resume functionality for an idle system. For
    stress-testing suspend/resume in parallel with other things going on, see
    power_SuspendStress.

    """
    version = 2

    def initialize(self):
        self._suspender = power_suspend.Suspender(self.resultsdir,
                method=sys_power.do_suspend, throw=True)


    def run_once(self):
        # Some idle time before initiating suspend-to-ram
        with chrome.Chrome():
            time.sleep(10)
            results = self._suspender.suspend(0)
            self.write_perf_keyval(results)
