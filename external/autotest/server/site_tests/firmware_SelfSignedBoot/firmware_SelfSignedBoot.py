# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.client.common_lib import error
from autotest_lib.server.cros import vboot_constants as vboot
from autotest_lib.server.cros.faft.firmware_test import FirmwareTest


class firmware_SelfSignedBoot(FirmwareTest):
    """
    Servo based developer mode boot only test to Self signed Kernels.

    This test requires a USB disk plugged-in, which contains a Chrome OS test
    image (built by 'build_image test'). On runtime, this test first switches
    DUT to dev mode. When dev_boot_usb=1 and dev_boot_signed_only=1, pressing
    Ctrl-U on developer screen should not boot the USB disk(recovery mode boot
    should work), and when USB image is resigned with SSD keys, pressing Ctrl-U
    should boot to the USB disk.
    """
    version = 1

    def initialize(self, host, cmdline_args, ec_wp=None):
        super(firmware_SelfSignedBoot, self).initialize(host, cmdline_args,
                                                        ec_wp=ec_wp)
        self.switcher.setup_mode('dev')
        self.setup_usbkey(usbkey=True, host=False)

        self.original_dev_boot_usb = self.faft_client.system.get_dev_boot_usb()
        logging.info('Original dev_boot_usb value: %s',
                     str(self.original_dev_boot_usb))

        self.usb_dev = self.get_usbdisk_path_on_dut()
        if not self.usb_dev:
            raise error.TestError("Unable to find USB disk")

    def cleanup(self):
        self.faft_client.system.set_dev_boot_usb(self.original_dev_boot_usb)
        self.disable_crossystem_selfsigned()
        self.ensure_internal_device_boot()
        self.resignimage_recoverykeys()
        super(firmware_SelfSignedBoot, self).cleanup()

    def ensure_internal_device_boot(self):
        """Ensure internal device boot; if not, reboot into it.

        If not, it may be a test failure during step 3 or 5, try to reboot
        and press Ctrl-D to internal device boot.
        """
        if self.faft_client.system.is_removable_device_boot():
            logging.info('Reboot into internal disk...')
            self.switcher.mode_aware_reboot()

    def resignimage_ssdkeys(self):
        """Re-signing the USB image using the SSD keys."""
        self.faft_client.system.run_shell_command(
            '/usr/share/vboot/bin/make_dev_ssd.sh -i %s' % self.usb_dev)

    def resignimage_recoverykeys(self):
        """Re-signing the USB image using the Recovery keys."""
        self.faft_client.system.run_shell_command(
            '/usr/share/vboot/bin/make_dev_ssd.sh -i %s --recovery_key'
            % self.usb_dev)

    def enable_crossystem_selfsigned(self):
        """Enable dev_boot_signed_only + dev_boot_usb."""
        self.faft_client.system.run_shell_command(
            'crossystem dev_boot_signed_only=1')
        self.faft_client.system.run_shell_command('crossystem dev_boot_usb=1')

    def disable_crossystem_selfsigned(self):
        """Disable dev_boot_signed_only + dev_boot_usb."""
        self.faft_client.system.run_shell_command(
            'crossystem dev_boot_signed_only=0')
        self.faft_client.system.run_shell_command('crossystem dev_boot_usb=0')

    def run_once(self):
        if (self.faft_config.has_keyboard and
                not self.check_ec_capability(['keyboard'])):
            raise error.TestNAError("TEST IT MANUALLY! This test can't be "
                                    "automated on non-Chrome-EC devices.")

        logging.info("Expected developer mode, set dev_boot_usb and "
                     "dev_boot_signed_only to 1.")
        self.check_state((self.checkers.dev_boot_usb_checker, False))
        self.enable_crossystem_selfsigned()
        self.switcher.mode_aware_reboot()

        logging.info("Expected internal disk boot, switch to recovery mode.")
        self.check_state((self.checkers.dev_boot_usb_checker, False,
                          'Not internal disk boot, dev_boot_usb misbehaved'))
        self.switcher.reboot_to_mode(to_mode='rec')

        logging.info("Expected recovery boot and reboot.")
        self.check_state((self.checkers.crossystem_checker, {
                   'mainfw_type': 'recovery',
                   'recovery_reason': vboot.RECOVERY_REASON['RO_MANUAL'],
                   }))
        self.switcher.mode_aware_reboot()

        logging.info("Expected internal disk boot, resign with SSD keys.")
        self.check_state((self.checkers.dev_boot_usb_checker, False,
                          'Not internal disk boot, dev_boot_usb misbehaved'))
        self.resignimage_ssdkeys()
        self.switcher.mode_aware_reboot(wait_for_dut_up=False)
        self.switcher.bypass_dev_boot_usb()
        self.switcher.wait_for_client()

        logging.info("Expected USB boot.")
        self.check_state((self.checkers.dev_boot_usb_checker, True,
                          'Not USB boot, Ctrl-U not work'))
        self.switcher.mode_aware_reboot()

        logging.info("Check and done.")
        self.check_state((self.checkers.dev_boot_usb_checker, False))
