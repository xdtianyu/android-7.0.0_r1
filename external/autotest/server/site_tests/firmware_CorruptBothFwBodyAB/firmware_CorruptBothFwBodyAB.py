# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.server.cros import vboot_constants as vboot
from autotest_lib.server.cros.faft.firmware_test import FirmwareTest


class firmware_CorruptBothFwBodyAB(FirmwareTest):
    """
    Servo based both firmware body A and B corruption test.

    The expected behavior is different if the firmware preamble USE_RO_NORMAL
    flag is enabled. In the case USE_RO_NORMAL ON, the firmware corruption
    doesn't hurt the boot results since it boots the RO path directly and does
    not load and verify the RW firmware body. In the case USE_RO_NORMAL OFF,
    the firmware verification fails on loading RW firmware and enters recovery
    mode. In this case, it requires a USB disk plugged-in, which contains a
    Chrome OS test image (built by "build_image --test").
    """
    version = 1

    use_ro = False

    def initialize(self, host, cmdline_args, dev_mode=False):
        super(firmware_CorruptBothFwBodyAB, self).initialize(host, cmdline_args)
        self.backup_firmware()
        if (self.faft_client.bios.get_preamble_flags('a') &
                vboot.PREAMBLE_USE_RO_NORMAL):
            self.use_ro = True
            self.switcher.setup_mode('dev' if dev_mode else 'normal')
        else:
            self.switcher.setup_mode('dev' if dev_mode else 'normal')
            self.setup_usbkey(usbkey=True, host=False)

    def cleanup(self):
        self.restore_firmware()
        super(firmware_CorruptBothFwBodyAB, self).cleanup()

    def run_once(self, dev_mode=False):
        if self.use_ro:
            # USE_RO_NORMAL flag is ON. Firmware body corruption doesn't
            # hurt the booting results.
            logging.info('The firmware USE_RO_NORMAL flag is enabled.')
            logging.info("Corrupt both firmware body A and B.")
            self.check_state((self.checkers.crossystem_checker, {
                        'mainfw_type': 'developer' if dev_mode else 'normal',
                        }))
            self.faft_client.bios.corrupt_body(('a', 'b'))
            self.switcher.mode_aware_reboot()

            logging.info("Still expected normal/developer boot and restore.")
            self.check_state((self.checkers.crossystem_checker, {
                        'mainfw_type': 'developer' if dev_mode else 'normal',
                        }))
            self.faft_client.bios.restore_body(('a', 'b'))
        else:
            logging.info("Corrupt both firmware body A and B.")
            self.check_state((self.checkers.crossystem_checker, {
                        'mainfw_type': 'developer' if dev_mode else 'normal',
                        }))
            self.faft_client.bios.corrupt_body(('a', 'b'))
            self.switcher.mode_aware_reboot()

            logging.info("Expected recovery boot and restore firmware.")
            self.check_state((self.checkers.crossystem_checker, {
                                  'mainfw_type': 'recovery',
                                  'recovery_reason':
                                  (vboot.RECOVERY_REASON['RO_INVALID_RW'],
                                  vboot.RECOVERY_REASON['RW_VERIFY_BODY']),
                                  }))
            self.faft_client.bios.restore_body(('a', 'b'))
            self.switcher.mode_aware_reboot()

            logging.info("Expected normal boot, done.")
            self.check_state((self.checkers.crossystem_checker, {
                        'mainfw_type': 'developer' if dev_mode else 'normal',
                        }))
