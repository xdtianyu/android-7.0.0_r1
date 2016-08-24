# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.server.cros.faft.firmware_test import FirmwareTest
from autotest_lib.server.cros.faft.firmware_test import ConnectionError


class firmware_CorruptFwSigB(FirmwareTest):
    """
    Servo based firmware signature B corruption test.
    """
    version = 1

    def initialize(self, host, cmdline_args, dev_mode=False):
        super(firmware_CorruptFwSigB, self).initialize(host, cmdline_args)
        self.backup_firmware()
        self.switcher.setup_mode('dev' if dev_mode else 'normal')
        self.setup_usbkey(usbkey=False)

    def cleanup(self):
        try:
            self.restore_firmware()
        except ConnectionError:
            logging.error("ERROR: DUT did not come up.  Need to cleanup!")
        super(firmware_CorruptFwSigB, self).cleanup()

    def run_once(self):
        logging.info("Expected firmware A boot and corrupt "
                     "firmware signature B.")
        self.check_state((self.checkers.fw_tries_checker, 'A'))
        self.faft_client.bios.corrupt_sig('b')
        self.switcher.mode_aware_reboot()

        logging.info("Expected firmware A boot and set try_fwb flag.")
        self.check_state((self.checkers.fw_tries_checker, 'A'))
        self.try_fwb()
        self.switcher.mode_aware_reboot()

        logging.info("Expected firmware A boot and restore firmware B.")
        self.check_state((self.checkers.fw_tries_checker, ('A', False)))
        self.faft_client.bios.restore_sig('b')
        self.switcher.mode_aware_reboot()

        logging.info("Final check and done.")
        self.check_state((self.checkers.fw_tries_checker, 'A'))
