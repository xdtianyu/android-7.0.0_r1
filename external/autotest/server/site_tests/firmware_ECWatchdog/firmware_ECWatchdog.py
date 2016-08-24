# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import time

from autotest_lib.client.common_lib import error
from autotest_lib.server.cros.faft.firmware_test import FirmwareTest

class firmware_ECWatchdog(FirmwareTest):
    """
    Servo based EC watchdog test.
    """
    version = 1


    # Delay of spin-wait in ms. Should be long enough to trigger watchdog reset.
    WATCHDOG_DELAY = 3000

    # Delay of EC power on.
    EC_BOOT_DELAY = 1000


    def initialize(self, host, cmdline_args):
        super(firmware_ECWatchdog, self).initialize(host, cmdline_args)
        # Only run in normal mode
        self.switcher.setup_mode('normal')


    def reboot_by_watchdog(self):
        """
        Trigger a watchdog reset.
        """
        self.faft_client.system.run_shell_command("sync")
        self.ec.send_command("waitms %d" % self.WATCHDOG_DELAY)
        time.sleep((self.WATCHDOG_DELAY + self.EC_BOOT_DELAY) / 1000.0)
        self.check_lid_and_power_on()


    def run_once(self):
        if not self.check_ec_capability():
            raise error.TestNAError("Nothing needs to be tested on this device")

        logging.info("Trigger a watchdog reset and power on system again.")
        self.switcher.mode_aware_reboot('custom', self.reboot_by_watchdog)
