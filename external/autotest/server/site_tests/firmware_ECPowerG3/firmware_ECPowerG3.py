# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.client.common_lib import error
from autotest_lib.server.cros.faft.firmware_test import FirmwareTest


class firmware_ECPowerG3(FirmwareTest):
    """
    Servo based EC X86 power G3 drop test.
    """
    version = 1

    # Time out range for waiting system drop into G3.
    G3_RETRIES = 13

    # Record failure event
    _failed = False

    def initialize(self, host, cmdline_args):
        super(firmware_ECPowerG3, self).initialize(host, cmdline_args)
        # Only run in normal mode
        self.switcher.setup_mode('normal')
        self.ec.send_command("chan 0")

    def cleanup(self):
        self.ec.send_command("chan 0xffffffff")
        super(firmware_ECPowerG3, self).cleanup()

    def check_G3(self):
        """Shutdown the system and check if X86 drop into G3 correctly."""
        self.faft_client.system.run_shell_command("shutdown -P now")
        if not self.wait_power_state("G3", self.G3_RETRIES):
            logging.error("EC fails to drop into G3")
            self._failed = True
        self.servo.power_short_press()

    def check_failure(self):
        """Check whether any failure has occurred."""
        return not self._failed

    def run_once(self):
        if not self.check_ec_capability(['x86']):
            raise error.TestNAError("Nothing needs to be tested on this device")

        logging.info("Power off and check if system drop into G3 correctly.")
        self.switcher.mode_aware_reboot('custom', self.check_G3)

        logging.info("Check if failure occurred.")
        self.check_state(self.check_failure)
