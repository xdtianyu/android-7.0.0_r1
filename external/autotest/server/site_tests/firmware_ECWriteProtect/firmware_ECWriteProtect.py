# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.client.common_lib import error
from autotest_lib.server.cros import vboot_constants as vboot
from autotest_lib.server.cros.faft.firmware_test import FirmwareTest


class firmware_ECWriteProtect(FirmwareTest):
    """
    Servo based EC write protect test.
    """
    version = 1

    def write_protect_checker(self):
        """Checker that ensure the following write protect flags are set:
            - wp_gpio_asserted
            - ro_at_boot
            - ro_now
            - all_now
        """
        try:
            self.ec.send_command_get_output("flashinfo",
                  ["Flags:\s+wp_gpio_asserted\s+ro_at_boot\s+ro_now\s+all_now"])
            return True
        except error.TestFail:
            # Didn't get expected flags
            return False

    def initialize(self, host, cmdline_args, dev_mode=False):
        super(firmware_ECWriteProtect, self).initialize(host, cmdline_args,
                                                        ec_wp=False)
        self.backup_firmware()
        self.switcher.setup_mode('dev' if dev_mode else 'normal')
        self.ec.send_command("chan 0")

    def cleanup(self):
        self.ec.send_command("chan 0xffffffff")
        self.restore_firmware()
        super(firmware_ECWriteProtect, self).cleanup()

    def run_once(self):
        flags = self.faft_client.bios.get_preamble_flags('a')
        if flags & vboot.PREAMBLE_USE_RO_NORMAL == 0:
            logging.info('The firmware USE_RO_NORMAL flag is disabled.')
            return

        logging.info("Expected EC RO boot, enable WP and reboot EC.")
        self.check_state((self.checkers.ro_normal_checker, 'A'))
        self.switcher.mode_aware_reboot(
                'custom', lambda:self.set_ec_write_protect_and_reboot(True))

        logging.info("Expected EC RO boot, write protected. Disable RO flag "
                     "and reboot EC.")
        self.check_state([(self.checkers.ro_normal_checker, 'A'),
                          self.write_protect_checker])
        self.faft_client.bios.set_preamble_flags('a', 0)
        self.switcher.mode_aware_reboot(reboot_type='cold')

        logging.info("Expected EC RW boot, write protected. Reboot EC by "
                     "ectool.")
        self.check_state((self.checkers.ro_normal_checker, ('A', True)))
        self.check_state(self.write_protect_checker)
        self.switcher.mode_aware_reboot(
                'custom', lambda:self.sync_and_ec_reboot('hard'))

        logging.info("Expected EC RW boot, write protected. Restore RO "
                     "normal flag and deactivate write protect.")
        self.check_state((self.checkers.ro_normal_checker, ('A', True)))
        self.check_state(self.write_protect_checker)
        self.faft_client.bios.set_preamble_flags(('a',
                                                  vboot.PREAMBLE_USE_RO_NORMAL))
        self.switcher.mode_aware_reboot(
                'custom', lambda:self.set_ec_write_protect_and_reboot(False))

        logging.info("Expected EC RO boot.")
        self.check_state((self.checkers.ro_normal_checker, 'A'))
