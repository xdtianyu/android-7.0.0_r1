# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.server.cros import vboot_constants as vboot
from autotest_lib.server.cros.faft.firmware_test import FirmwareTest
from autotest_lib.server.cros.faft.firmware_test import ConnectionError


class firmware_CorruptFwBodyB(FirmwareTest):
    """
    Servo based firmware body B corruption test.

    The expected behavior is different if the firmware preamble USE_RO_NORMAL
    flag is enabled. In the case USE_RO_NORMAL ON, the firmware corruption
    doesn't hurt the boot results since it boots the RO path directly and does
    not load and verify the RW firmware body. In the case USE_RO_NORMAL OFF,
    the RW firwmare B corruption will result booting the firmware A.
    """
    version = 1

    def initialize(self, host, cmdline_args, dev_mode=False):
        super(firmware_CorruptFwBodyB, self).initialize(host, cmdline_args)
        self.backup_firmware()
        self.switcher.setup_mode('dev' if dev_mode else 'normal')
        self.setup_usbkey(usbkey=False)

    def cleanup(self):
        try:
            self.restore_firmware()
        except ConnectionError:
            logging.error("ERROR: DUT did not come up.  Need to cleanup!")
        super(firmware_CorruptFwBodyB, self).cleanup()

    def run_once(self):
        RO_enabled = (self.faft_client.bios.get_preamble_flags('b') &
                      vboot.PREAMBLE_USE_RO_NORMAL)
        logging.info("Corrupt firmware body B.")
        self.check_state((self.checkers.fw_tries_checker, 'A'))
        self.faft_client.bios.corrupt_body('b')
        self.switcher.mode_aware_reboot()

        logging.info("Expected firmware A boot and set try_fwb flag.")
        self.check_state((self.checkers.fw_tries_checker, 'A'))
        self.try_fwb(1)
        self.switcher.mode_aware_reboot()

        logging.info("If RO enabled, expected firmware B boot; otherwise, "
                     "still A boot since B is corrupted. Restore B later.")
        if RO_enabled:
            self.check_state((self.checkers.fw_tries_checker, 'B'))
        else:
            self.check_state((self.checkers.fw_tries_checker, ('A', False)))
        self.faft_client.bios.restore_body('b')
        self.switcher.mode_aware_reboot()

        logging.info("Final check and done.")
        self.check_state((self.checkers.fw_tries_checker, 'A'))
