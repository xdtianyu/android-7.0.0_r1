# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.client.common_lib import error
from autotest_lib.server.cros.faft.firmware_test import FirmwareTest


class firmware_DevBootUSB(FirmwareTest):
    """
    Servo based Ctrl-U developer USB boot test.

    This test requires a USB disk plugged-in, which contains a Chrome OS test
    image (built by "build_image test"). On runtime, this test first switches
    DUT to developer mode. When dev_boot_usb=0, pressing Ctrl-U on developer
    screen should not boot the USB disk. When dev_boot_usb=1, pressing Ctrl-U
    should boot the USB disk.
    """
    version = 1

    def initialize(self, host, cmdline_args, ec_wp=None):
        super(firmware_DevBootUSB, self).initialize(host, cmdline_args,
                                                    ec_wp=ec_wp)
        self.switcher.setup_mode('dev')
        self.setup_usbkey(usbkey=True, host=False)

        self.original_dev_boot_usb = self.faft_client.system.get_dev_boot_usb()
        logging.info('Original dev_boot_usb value: %s',
                     str(self.original_dev_boot_usb))

    def cleanup(self):
        self.ensure_internal_device_boot()
        super(firmware_DevBootUSB, self).cleanup()

    def ensure_internal_device_boot(self):
        """Ensure internal device boot; if not, reboot into it.

        If not, it may be a test failure during step 2 or 3, try to reboot
        and press Ctrl-D to internal device boot.
        """
        if self.faft_client.system.is_removable_device_boot():
            logging.info('Reboot into internal disk...')
            self.faft_client.system.set_dev_boot_usb(self.original_dev_boot_usb)
            self.switcher.mode_aware_reboot()

    def run_once(self):
        if (self.faft_config.has_keyboard and
                not self.check_ec_capability(['keyboard'])):
            raise error.TestNAError("TEST IT MANUALLY! This test can't be "
                                    "automated on non-Chrome-EC devices.")

        logging.info("Expected developer mode, set dev_boot_usb to 0.")
        self.check_state((self.checkers.dev_boot_usb_checker, False))
        self.faft_client.system.set_dev_boot_usb(0)
        self.switcher.mode_aware_reboot()

        logging.info("Expected internal disk boot, set dev_boot_usb to 1.")
        self.check_state((self.checkers.dev_boot_usb_checker,
                          False,
                          "Not internal disk boot, dev_boot_usb misbehaved"))
        self.faft_client.system.set_dev_boot_usb(1)
        self.switcher.mode_aware_reboot(wait_for_dut_up=False)
        self.switcher.bypass_dev_boot_usb()
        self.switcher.wait_for_client()

        logging.info("Expected USB boot, set dev_boot_usb to the original.")
        self.check_state((self.checkers.dev_boot_usb_checker,
                          True,
                          "Not USB boot, Ctrl-U not work"))
        self.faft_client.system.set_dev_boot_usb(self.original_dev_boot_usb)
        self.switcher.mode_aware_reboot(wait_for_dut_up=False)
        self.switcher.bypass_dev_mode()
        self.switcher.wait_for_client()

        logging.info("Check and done.")
        self.check_state((self.checkers.dev_boot_usb_checker, False))
