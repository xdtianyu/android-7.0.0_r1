# Copyright (c) 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import time
import re

from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.server.cros import vboot_constants as vboot
from autotest_lib.server.cros.faft.firmware_test import FirmwareTest


class firmware_FastbootErase(FirmwareTest):
    """
    Testing Erase functionality in Fastboot
    Will erase the kernel, causing the DUT to
    boot into recovery mode.  Then restores the
    kernel.

    This needs to be only enabled for Android tests.
    """
    version = 1

    KERNEL_TMP_FILE = '/tmp/android_kernel'

    def initialize(self, host, cmdline_args, dev_mode=False):
        super(firmware_FastbootErase, self).initialize(host, cmdline_args)
        self.switcher.setup_mode('dev' if dev_mode else 'normal')
        # set flags so that we can do the erase command
        self.clear_set_gbb_flags(0,
                                 vboot.GBB_FLAG_FORCE_DEV_BOOT_FASTBOOT_FULL_CAP)

    def cleanup(self):
        """Restore the gbb flags"""
        self.clear_set_gbb_flags(vboot.GBB_FLAG_FORCE_DEV_BOOT_FASTBOOT_FULL_CAP,
                                 0)
        super(firmware_FastbootErase, self).cleanup()

    def is_recovery_mode(self):
        """Check if DUT in recovery mode

        Unfortunately, adb get-state just returns "unknown"
        so need to do it with adb devices and regexp
        """
        result = self.faft_client.host.run_shell_command_get_output(
            'adb devices')
        if 'recovery' in result[1]:
            return True
        else:
            return False

    def wait_for_client_recovery(self, timeout=300):
        """Blocks until detects that DUT is in recovery mode"""
        utils.wait_for_value(self.is_recovery_mode, True, timeout_sec=timeout)

    def run_once(self, dev_mode=False):
        if not self.faft_client.system.has_host():
            raise error.TestNAError('DUT is not Android device.  Skipping test')

        # first copy out kernel partition
        logging.info("Making copy of kernel")
        self.faft_client.host.run_shell_command(
            'adb pull /dev/block/mmcblk0p1 %s' % self.KERNEL_TMP_FILE)

        # now erase boot partition
        self.faft_client.host.run_shell_command('adb reboot bootloader')
        self.switcher.wait_for_client_fastboot()

        if not self.switcher.is_fastboot_mode():
            raise error.TestFail("DUT not in fastboot mode!")

        logging.info("Erasing the kernel")
        self.faft_client.host.run_shell_command('fastboot erase kernel')
        self.faft_client.host.run_shell_command('fastboot continue')

        # DUT should enter into recovery OS
        self.wait_for_client_recovery()

        # Push the volume down button to "Reboot to bootloader"
        # and select it with the power button
        self.servo.volume_down()
        self.servo.power_key(self.faft_config.hold_pwr_button_poweron)
        self.switcher.wait_for_client_fastboot()

        # Should be in recovery mode.
        # Reflash the kernel image that we saved earlier
        logging.info("Repairing the kernel")
        self.faft_client.host.run_shell_command(
            'fastboot flash kernel %s' % self.KERNEL_TMP_FILE)
        self.faft_client.host.run_shell_command(
            'fastboot continue')

        self.switcher.wait_for_client()

        # cleaning up the temp file
        self.faft_client.host.run_shell_command(
            'rm %s' % self.KERNEL_TMP_FILE)
