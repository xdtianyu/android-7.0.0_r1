# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.server.cros import vboot_constants as vboot
from autotest_lib.server.cros.faft.firmware_test import FirmwareTest


class firmware_UserRequestRecovery(FirmwareTest):
    """
    Servo based user request recovery boot test.

    This test requires a USB disk plugged-in, which contains a Chrome OS test
    image (built by "build_image --test"). On runtime, this test first requests
    a recovery mode on next boot by setting the crossystem recovery_request
    flag. It then triggers recovery mode by unplugging and plugging in the USB
    disk and checks success of it.
    """
    version = 1

    def ensure_normal_boot(self):
        """Ensure normal mode boot this time.

        If not, it may be a test failure during step 2, try to recover to
        normal mode by simply rebooting the machine.
        """
        if not self.checkers.crossystem_checker(
                {'mainfw_type': ('normal', 'developer')}):
            self.switcher.mode_aware_reboot()

    def initialize(self, host, cmdline_args, dev_mode=False, ec_wp=None):
        super(firmware_UserRequestRecovery, self).initialize(host, cmdline_args,
                                                             ec_wp=ec_wp)
        self.switcher.setup_mode('dev' if dev_mode else 'normal')
        self.setup_usbkey(usbkey=True, host=True)

    def cleanup(self):
        self.ensure_normal_boot()
        super(firmware_UserRequestRecovery, self).cleanup()

    def run_once(self, dev_mode=False):
        logging.info("Request recovery boot.")
        self.check_state((self.checkers.crossystem_checker, {
                           'mainfw_type': 'developer' if dev_mode else 'normal',
                           }))
        self.faft_client.system.request_recovery_boot()
        self.switcher.mode_aware_reboot(wait_for_dut_up=False)
        self.switcher.bypass_rec_mode()
        self.switcher.wait_for_client()

        logging.info("Expected recovery boot, request recovery again.")
        self.check_state((self.checkers.crossystem_checker, {
                           'mainfw_type': 'recovery',
                           'recovery_reason' : vboot.RECOVERY_REASON['US_TEST'],
                           }))
        self.faft_client.system.request_recovery_boot()
        self.switcher.mode_aware_reboot(wait_for_dut_up=False)
        if not dev_mode:
            self.switcher.bypass_rec_mode()
        self.switcher.wait_for_client()

        logging.info("Expected recovery boot.")
        self.check_state((self.checkers.crossystem_checker, {
                           'mainfw_type': 'recovery',
                           'recovery_reason' : vboot.RECOVERY_REASON['US_TEST'],
                           }))
        self.switcher.mode_aware_reboot()

        logging.info("Expected normal boot.")
        self.check_state((self.checkers.crossystem_checker, {
                           'mainfw_type': 'developer' if dev_mode else 'normal',
                           }))
