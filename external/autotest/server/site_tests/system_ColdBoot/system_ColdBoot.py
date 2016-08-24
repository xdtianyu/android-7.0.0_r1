# Copyright (c) 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import time

from autotest_lib.server import autotest, test

class system_ColdBoot(test.test):
    """
    Shut down the device gracefully via Linux shell commands, then simulate
    a power button press and verify that it comes back up correctly.
    """
    version = 1

    # Allowed timeout for graceful shutdown.
    TIMEOUT_POWEROFF_TRANSITION = 15
    # Time to sleep to ensure full power off, after OS quits replying to pings.
    WAIT_TIME_FULL_POWEROFF = 5

    def run_once(self, host):
        boot_id = host.get_boot_id()

        host.run("poweroff")
        host.test_wait_for_shutdown(self.TIMEOUT_POWEROFF_TRANSITION)
        time.sleep(self.WAIT_TIME_FULL_POWEROFF)

        host.servo.power_normal_press()
        host.test_wait_for_boot(boot_id)

        autotest.Autotest(host).run_test("desktopui_SimpleLogin",
                                         exit_without_logout=True)
