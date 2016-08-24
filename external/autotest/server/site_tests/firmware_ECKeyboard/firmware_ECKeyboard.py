# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import time

from autotest_lib.client.common_lib import error
from autotest_lib.server.cros.faft.firmware_test import FirmwareTest


class firmware_ECKeyboard(FirmwareTest):
    """
    Servo based EC keyboard test.
    """
    version = 1

    # Delay between commands
    CMD_DELAY = 1

    def initialize(self, host, cmdline_args):
        super(firmware_ECKeyboard, self).initialize(host, cmdline_args)
        # Only run in normal mode
        self.switcher.setup_mode('normal')

    def switch_tty2(self):
        """Switch to tty2 console."""
        self.ec.key_down('<ctrl_l>')
        self.ec.key_down('<alt_l>')
        self.ec.key_down('<f2>')
        self.ec.key_up('<f2>')
        self.ec.key_up('<alt_l>')
        self.ec.key_up('<ctrl_l>')
        time.sleep(self.CMD_DELAY)

    def reboot_by_keyboard(self):
        """
        Simulate key press sequence to log into console and then issue reboot
        command.
        """
        self.switch_tty2()
        self.ec.send_key_string('root<enter>')
        time.sleep(self.CMD_DELAY)
        self.ec.send_key_string('test0000<enter>')
        time.sleep(self.CMD_DELAY)
        self.ec.send_key_string('reboot<enter>')

    def run_once(self):
        if not self.check_ec_capability(['keyboard']):
            raise error.TestNAError("Nothing needs to be tested on this device")

        logging.info("Use key press simulation to issue reboot command.")
        self.switcher.mode_aware_reboot('custom', self.reboot_by_keyboard)
