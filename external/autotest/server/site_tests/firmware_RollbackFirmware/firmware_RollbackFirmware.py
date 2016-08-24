# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.server.cros import vboot_constants as vboot
from autotest_lib.server.cros.faft.firmware_test import FirmwareTest


class firmware_RollbackFirmware(FirmwareTest):
    """
    Servo based firmware rollback test.

    This test requires a USB disk plugged-in, which contains a Chrome OS test
    image (built by "build_image --test"). On runtime, this test rollbacks
    firmware A and results firmware B boot. It then rollbacks firmware B and
    results recovery boot.
    """
    version = 1

    def initialize(self, host, cmdline_args, dev_mode=False):
        super(firmware_RollbackFirmware, self).initialize(host, cmdline_args)
        self.backup_firmware()
        self.switcher.setup_mode('dev' if dev_mode else 'normal')
        self.setup_usbkey(usbkey=True, host=False)

    def cleanup(self):
        self.restore_firmware()
        super(firmware_RollbackFirmware, self).cleanup()

    def run_once(self, dev_mode=False):
        logging.info("Rollback firmware A.")
        self.check_state((self.checkers.fw_tries_checker, 'A'))
        self.faft_client.bios.move_version_backward('a')
        self.switcher.mode_aware_reboot()

        logging.info("Expected firmware B boot and rollback firmware B.")
        self.check_state((self.checkers.fw_tries_checker, ('B', False)))
        self.faft_client.bios.move_version_backward('b')
        self.switcher.mode_aware_reboot()

        logging.info("Expected recovery boot and restores firmware A and B.")
        self.check_state((self.checkers.crossystem_checker, {
                           'mainfw_type': 'recovery',
                           'recovery_reason': (
                                vboot.RECOVERY_REASON['RO_INVALID_RW'],
                                vboot.RECOVERY_REASON['RW_FW_ROLLBACK']),
                           }))
        self.faft_client.bios.move_version_forward(('a', 'b'))
        self.switcher.mode_aware_reboot()

        expected_slot = 'B' if self.fw_vboot2 else 'A'
        logging.info("Expected firmware " + expected_slot + " boot, done.")
        self.check_state((self.checkers.fw_tries_checker, expected_slot))
