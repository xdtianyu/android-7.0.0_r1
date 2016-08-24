# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.server.cros import vboot_constants as vboot
from autotest_lib.server.cros.faft.firmware_test import FirmwareTest
from autotest_lib.server.cros.faft.firmware_test import ConnectionError


class firmware_CorruptFwBodyA(FirmwareTest):
    """
    Servo based firmware body A corruption test.

    The expected behavior is different if the firmware preamble USE_RO_NORMAL
    flag is enabled. In the case USE_RO_NORMAL ON, the firmware corruption
    doesn't hurt the boot results since it boots the RO path directly and does
    not load and verify the RW firmware body. In the case USE_RO_NORMAL OFF,
    the RW firwmare A corruption will result booting the firmware B.
    """
    version = 1

    def initialize(self, host, cmdline_args, dev_mode=False):
        super(firmware_CorruptFwBodyA, self).initialize(host, cmdline_args)
        self.backup_firmware()
        self.switcher.setup_mode('dev' if dev_mode else 'normal')
        self.setup_usbkey(usbkey=False)

    def cleanup(self):
        try:
            self.restore_firmware()
        except ConnectionError:
            logging.error("ERROR: DUT did not come up.  Need to cleanup!")
        super(firmware_CorruptFwBodyA, self).cleanup()

    def run_once(self):
        if (self.faft_client.bios.get_preamble_flags('a') &
                vboot.PREAMBLE_USE_RO_NORMAL):
            # USE_RO_NORMAL flag is ON. Firmware body corruption doesn't
            # hurt the booting results.
            logging.info('The firmware USE_RO_NORMAL flag is enabled.')
            logging.info("Corrupt firmware body A.")
            self.check_state((self.checkers.fw_tries_checker, 'A'))
            self.faft_client.bios.corrupt_body('a')
            self.switcher.mode_aware_reboot()

            logging.info("Still expected firmware A boot and restore.")
            self.check_state((self.checkers.fw_tries_checker, 'A'))
            self.faft_client.bios.restore_body('a')
        else:
            logging.info('The firmware USE_RO_NORMAL flag is disabled.')
            logging.info("Corrupt firmware body A.")
            self.check_state((self.checkers.fw_tries_checker, 'A'))
            self.faft_client.bios.corrupt_body('a')
            self.switcher.mode_aware_reboot()

            logging.info("Expected firmware B boot and restore firmware A.")
            self.check_state((self.checkers.fw_tries_checker, ('B', False)))
            self.faft_client.bios.restore_body('a')
            self.switcher.mode_aware_reboot()

            expected_slot = 'B' if self.fw_vboot2 else 'A'
            logging.info("Expected firmware " + expected_slot + " boot, done.")
            self.check_state((self.checkers.fw_tries_checker, expected_slot))
