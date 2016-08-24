# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import time

from autotest_lib.client.common_lib import error
from autotest_lib.server.cros.faft.firmware_test import FirmwareTest


class firmware_ECKeyboardReboot(FirmwareTest):
    """
    Test the dut-control ec_uart_cmd:reboot command.
    This simulate the Power+refresh reboot but not exactly.  The F3 + power EC
    reset is triggered by the Silego IC, and it taps directly into the KB row
    column lines to check the trigger (requires physical presence).

    see test case: 1.3.8 Power+refresh; System reboots
    https://testtracker.googleplex.com/efforts/testcase/detail/721602
    """
    version = 1

    # Delay between commands
    CMD_DELAY = 1

    def initialize(self, host, cmdline_args):
        super(firmware_ECKeyboardReboot, self).initialize(host, cmdline_args)
        # Only run in normal mode
        self.switcher.setup_mode('normal')
        self.host = host

    def confirm_dut_off(self):
        if not self.host.ping_wait_down(timeout=10):
          raise error.TestFail('DUT is on, expected off')
        logging.info('DUT is off as expected')

    def confirm_dut_on(self):
        if not self.host.wait_up(timeout=30):
          raise error.TestFail('DUT is off, expected on')
        logging.info('DUT is on as expected')

    def run_once(self):
        if not self.check_ec_capability(['keyboard']):
          raise error.TestNAError("Nothing needs to be tested on this device")
        logging.info("Test dut-control ec_uart_cmd:reboot command.")

        self.ec.reboot()
        self.confirm_dut_off()
        self.confirm_dut_on()

        self.ec.reboot('hard')
        self.confirm_dut_off()
        self.confirm_dut_on()

        self.ec.reboot('ap-off')
        self.confirm_dut_off()
        self.ec.reboot()
        self.confirm_dut_on()
