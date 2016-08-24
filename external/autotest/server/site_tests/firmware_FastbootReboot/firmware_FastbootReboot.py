# Copyright (c) 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import time

from autotest_lib.client.common_lib import error
from autotest_lib.server.cros.faft.firmware_test import FirmwareTest


class firmware_FastbootReboot(FirmwareTest):
    """
    Reboot testing through Fastboot.
    Testing:
      fastboot reboot
      fastboot reboot-bootloader

    This needs to be only enabled for Android tests.
    """
    version = 1

    def initialize(self, host, cmdline_args, dev_mode=False):
        super(firmware_FastbootReboot, self).initialize(host, cmdline_args)
        self.switcher.setup_mode('dev' if dev_mode else 'normal')

    def in_fastboot_mode(self):
        # make sure that we're in fastboot mode
        result = self.faft_client.host.run_shell_command_get_output(
            'fastboot devices')
        if not result:
            return False
        else:
            return True

    def run_once(self, dev_mode=False):
        if not self.faft_client.system.has_host():
            raise error.TestNAError('DUT is not Android device.  Skipping test')

        self.faft_client.host.run_shell_command('adb reboot bootloader')
        # make sure that DUT goes offline first
        self.switcher.wait_for_client_offline()
        self.switcher.wait_for_client_fastboot()

        if not self.in_fastboot_mode():
            raise error.TestFail("DUT not in fastboot mode!")

        # try rebooting into OS
        logging.info("Testing fastboot reboot")
        self.faft_client.host.run_shell_command('fastboot reboot')
        # make sure that DUT goes offline first
        self.switcher.wait_for_client_offline()
        self.switcher.wait_for_client()

        # now reboot into fastboot again
        self.faft_client.host.run_shell_command('adb reboot bootloader')
        # make sure that DUT goes offline first
        self.switcher.wait_for_client_offline()
        self.switcher.wait_for_client_fastboot()
        if not self.in_fastboot_mode():
            raise error.TestFail("DUT not in fastboot mode!")

        logging.info("Testing fastboot reboot-bootloader")
        self.faft_client.host.run_shell_command('fastboot reboot-bootloader')
        # make sure that DUT goes offline first
        self.switcher.wait_for_client_offline()
        self.switcher.wait_for_client_fastboot()
        if not self.in_fastboot_mode():
            raise error.TestFail("DUT not in fastboot mode!")

        self.faft_client.host.run_shell_command('fastboot continue')
        self.switcher.wait_for_client()
