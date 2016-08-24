# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.client.common_lib import error
from autotest_lib.server.cros import vboot_constants as vboot
from autotest_lib.server.cros.faft.firmware_test import ConnectionError
from autotest_lib.server.cros.faft.firmware_test import FirmwareTest


class firmware_LegacyRecovery(FirmwareTest):
    """
    Servo based test to Verify recovery request at Remove Screen.

    This test requires a USB disk plugged-in, which contains a Chrome OS test
    image (built by "build_image --test"). It recovery boots to the USB image
    and sets recovery_request=1 and do a reboot. A failure is expected.
    """
    version = 1

    def initialize(self, host, cmdline_args):
        super(firmware_LegacyRecovery, self).initialize(host, cmdline_args)
        self.setup_usbkey(usbkey=True, host=False)
        self.switcher.setup_mode('normal')

    def cleanup(self):
        super(firmware_LegacyRecovery, self).cleanup()

    def run_once(self):
        logging.info("Turn on the recovery boot. Enable recovery request "
                     "and perform a reboot.")
        self.check_state((self.checkers.crossystem_checker, {
                           'devsw_boot': '0',
                           'mainfw_type': 'normal',
                           }))
        self.faft_client.system.request_recovery_boot()
        self.switcher.mode_aware_reboot(wait_for_dut_up=False)
        self.switcher.bypass_rec_mode()
        try:
            self.switcher.wait_for_client()
        except ConnectionError:
            raise error.TestError('Failed to boot the USB image.')
        self.faft_client.system.run_shell_command(
                                   'crossystem recovery_request=1')

        logging.info("Wait to ensure no recovery boot at remove screen "
                     "and a boot failure is expected. "
                     "Unplug and plug USB, try to boot it again.")
        self.check_state((self.checkers.crossystem_checker, {
                           'mainfw_type': 'recovery',
                           }))
        self.switcher.mode_aware_reboot(wait_for_dut_up=False)
        logging.info('Wait to ensure DUT doesnt Boot on USB at Remove screen.')
        try:
            self.switcher.wait_for_client()
            raise error.TestFail('Unexpected USB boot at Remove Screen.')
        except ConnectionError:
            logging.info('Done, Waited till timeout and no USB boot occured.')
        self.switcher.bypass_rec_mode()
        self.switcher.wait_for_client()

        logging.info("Expected to boot the restored USB image and reboot.")
        self.check_state((self.checkers.crossystem_checker, {
                           'mainfw_type': 'recovery',
                           'recovery_reason': vboot.RECOVERY_REASON['LEGACY'],
                           }))
        self.switcher.mode_aware_reboot()

        logging.info("Expected to normal boot and done.")
        self.check_state((self.checkers.crossystem_checker, {
                           'devsw_boot': '0',
                           'mainfw_type': 'normal',
                           }))
