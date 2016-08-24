# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import time
import sys

from autotest_lib.client.common_lib import error
from autotest_lib.server.cros.faft.firmware_test import FirmwareTest


class firmware_FwScreenPressPower(FirmwareTest):
    """
    Servo based power button triggered shutdown test during firmware screens.

    This test requires a USB disk plugged-in, which contains a Chrome OS test
    image (built by "build_image --test"). On runtime, this test triggers
    firmware screens (developer, remove, insert, yuck, and to_norm screens),
    and then presses the power button in order to power the machine down.
    """
    version = 1

    SHORT_SHUTDOWN_CONFIRMATION_PERIOD = 0.1

    def wait_fw_screen_and_press_power(self):
        """Wait for firmware warning screen and press power button."""
        time.sleep(self.faft_config.firmware_screen)
        # While the firmware screen, the power button probing loop sleeps
        # 0.25 second on every scan. Use the normal delay (1.2 second) for
        # power press.
        self.servo.power_normal_press()

    def wait_longer_fw_screen_and_press_power(self):
        """Wait for firmware screen without timeout and press power button."""
        time.sleep(self.faft_config.dev_screen_timeout)
        self.wait_fw_screen_and_press_power()

    def wait_second_screen_and_press_power(self):
        """Wait and trigger a second screen and press power button."""
        self.switcher.trigger_dev_to_rec()
        self.wait_longer_fw_screen_and_press_power()

    def wait_yuck_screen_and_press_power(self):
        """Insert corrupted USB for yuck screen and press power button."""
        # This USB stick will be removed in cleanup phase.
        self.servo.switch_usbkey('dut')
        time.sleep(self.faft_config.usb_plug)
        self.wait_longer_fw_screen_and_press_power()

    def initialize(self, host, cmdline_args):
        super(firmware_FwScreenPressPower, self).initialize(host, cmdline_args)
        self.assert_test_image_in_usb_disk()
        self.switcher.setup_mode('dev')
        self.servo.switch_usbkey('host')
        usb_dev = self.servo.probe_host_usb_dev()
        # Corrupt the kernel of USB stick. It is needed for triggering a
        # yuck screen later.
        self.corrupt_usb_kernel(usb_dev)

    def cleanup(self):
        self.servo.switch_usbkey('host')
        usb_dev = self.servo.probe_host_usb_dev()
        # Restore the kernel of USB stick which is corrupted on setup phase.
        self.restore_usb_kernel(usb_dev)
        super(firmware_FwScreenPressPower, self).cleanup()

    def run_once(self):
        if self.faft_config.fw_bypasser_type != 'ctrl_d_bypasser':
            raise error.TestNAError("This test is only valid on devices with "
                                    "screens.")
        if not self.faft_config.has_powerbutton:
            raise error.TestNAError("This test is only valid on devices with "
                                    "power button.")

        logging.info("Expected dev mode and reboot. "
                     "When the next DEVELOPER SCREEN shown, press power button "
                     "to make DUT shutdown.")
        self.check_state((self.checkers.crossystem_checker, {
                              'devsw_boot': '1',
                              'mainfw_type': 'developer',
                              }))
        self.switcher.mode_aware_reboot(wait_for_dut_up=False)
        self.run_shutdown_process(self.wait_fw_screen_and_press_power,
                                  post_power_action=self.switcher.bypass_dev_mode)
        self.switcher.wait_for_client()

        logging.info("Reboot. When the developer screen shown, press "
                     "enter key to trigger either TO_NORM screen (new) or "
                     "RECOVERY INSERT screen (old). Then press power button to "
                     "make DUT shutdown.")
        self.check_state((self.checkers.crossystem_checker, {
                              'devsw_boot': '1',
                              'mainfw_type': 'developer',
                              }))
        self.switcher.mode_aware_reboot(wait_for_dut_up=False)
        self.run_shutdown_process(self.wait_second_screen_and_press_power,
                                  post_power_action=self.switcher.bypass_dev_mode,
                                  shutdown_timeout=self.SHORT_SHUTDOWN_CONFIRMATION_PERIOD)
        self.switcher.wait_for_client()

        logging.info("Request recovery boot. When the RECOVERY INSERT "
                     "screen shows, press power button to make DUT shutdown.")
        self.check_state((self.checkers.crossystem_checker, {
                              'devsw_boot': '1',
                              'mainfw_type': 'developer',
                              }))
        self.faft_client.system.request_recovery_boot()
        self.switcher.mode_aware_reboot(wait_for_dut_up=False)
        self.run_shutdown_process(self.wait_longer_fw_screen_and_press_power,
                                  post_power_action=self.switcher.bypass_dev_mode,
                                  shutdown_timeout=self.SHORT_SHUTDOWN_CONFIRMATION_PERIOD)
        self.switcher.wait_for_client()

        logging.info("Request recovery boot again. When the recovery "
                     "insert screen shows, insert a corrupted USB and trigger "
                     "a YUCK SCREEN. Then press power button to "
                     "make DUT shutdown.")
        self.check_state((self.checkers.crossystem_checker, {
                              'devsw_boot': '1',
                              'mainfw_type': 'developer',
                              }))
        self.faft_client.system.request_recovery_boot()
        self.switcher.mode_aware_reboot(wait_for_dut_up=False)
        self.run_shutdown_process(self.wait_yuck_screen_and_press_power,
                                  post_power_action=self.switcher.bypass_dev_mode,
                                  shutdown_timeout=self.SHORT_SHUTDOWN_CONFIRMATION_PERIOD)
        self.switcher.wait_for_client()

        logging.info("Switch back to normal mode.")
        self.check_state((self.checkers.crossystem_checker, {
                              'devsw_boot': '1',
                              'mainfw_type': 'developer',
                              }))
        self.switcher.reboot_to_mode(to_mode='normal')

        logging.info("Expected normal mode and request recovery boot. "
                     "Because an USB stick is inserted, a RECOVERY REMOVE "
                     "screen shows. Press power button to make DUT shutdown.")
        self.check_state((self.checkers.crossystem_checker, {
                              'devsw_boot': '0',
                              'mainfw_type': 'normal',
                              }))
        self.faft_client.system.request_recovery_boot()
        self.switcher.mode_aware_reboot(wait_for_dut_up=False)
        self.run_shutdown_process(self.wait_longer_fw_screen_and_press_power,
                                  shutdown_timeout=self.SHORT_SHUTDOWN_CONFIRMATION_PERIOD)
        self.switcher.wait_for_client()

        logging.info("Check and done.")
        self.check_state((self.checkers.crossystem_checker, {
                              'devsw_boot': '0',
                              'mainfw_type': 'normal',
                              }))
