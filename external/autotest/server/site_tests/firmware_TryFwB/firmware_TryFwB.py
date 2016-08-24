# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
from autotest_lib.server.cros.faft.firmware_test import FirmwareTest


class firmware_TryFwB(FirmwareTest):
    """
    Servo based RW firmware B boot test.
    """
    version = 1

    def initialize(self, host, cmdline_args, dev_mode=False, ec_wp=None):
        super(firmware_TryFwB, self).initialize(host, cmdline_args, ec_wp=ec_wp)
        self.switcher.setup_mode('dev' if dev_mode else 'normal')
        self.setup_usbkey(usbkey=False)
        if not self.fw_vboot2:
            self.setup_tried_fwb(tried_fwb=False)

    def cleanup(self):
        self.setup_tried_fwb(tried_fwb=False)
        super(firmware_TryFwB, self).cleanup()

    def run_once(self):
        logging.info("Set fwb_tries flag")
        self.check_state((self.checkers.fw_tries_checker, 'A'))
        self.try_fwb()
        self.switcher.mode_aware_reboot()

        logging.info("Expected firmware B boot, reboot")
        self.check_state((self.checkers.fw_tries_checker, 'B'))
        self.switcher.mode_aware_reboot()

        expected_slot = 'B' if self.fw_vboot2 else 'A'
        logging.info("Expected firmware " + expected_slot + " boot, done.")
        self.check_state((self.checkers.fw_tries_checker, expected_slot))
