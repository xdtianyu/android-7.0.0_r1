# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.server.cros import vboot_constants as vboot
from autotest_lib.server.cros.faft.firmware_test import FirmwareTest


class firmware_RONormalBoot(FirmwareTest):
    """
    Servo based firmware RO normal boot test.

    This test only runs on the firmware on which its firmware preamble flags
    have USE_RO_NORMAL enabled. Since we always build and pack a workable
    RW firmware in the RW firmware body section, although it is not used when
    the USE_RO_NORMAL flag is enabled.

    On runtime, the test disables the RO normal boot flag in the current
    firmware and checks its next boot result.
    """
    version = 1

    def initialize(self, host, cmdline_args, dev_mode=False, ec_wp=None):
        super(firmware_RONormalBoot, self).initialize(host, cmdline_args,
                                                      ec_wp=ec_wp)
        self.backup_firmware()
        self.switcher.setup_mode('dev' if dev_mode else 'normal')
        self.setup_usbkey(usbkey=False)

    def cleanup(self):
        self.restore_firmware()
        super(firmware_RONormalBoot, self).cleanup()

    def run_once(self):
        flags = self.faft_client.bios.get_preamble_flags('a')
        if flags & vboot.PREAMBLE_USE_RO_NORMAL == 0:
            logging.info('The firmware USE_RO_NORMAL flag is disabled.')
            return

        logging.info("Disable the RO normal boot flag.")
        self.check_state((self.checkers.ro_normal_checker, 'A'))
        self.faft_client.bios.set_preamble_flags(('a',
                                      flags ^ vboot.PREAMBLE_USE_RO_NORMAL))
        self.switcher.mode_aware_reboot()

        logging.info("Expected TwoStop boot, restore the original flags.")
        self.check_state((lambda: self.checkers.ro_normal_checker('A',
                                                                  twostop=True)))
        self.faft_client.bios.set_preamble_flags('a', flags)
        self.switcher.mode_aware_reboot()
        self.check_state((self.checkers.ro_normal_checker, 'A'))
