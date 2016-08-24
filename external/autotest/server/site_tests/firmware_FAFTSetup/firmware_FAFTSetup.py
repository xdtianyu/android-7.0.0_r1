# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
from threading import Timer

from autotest_lib.client.common_lib import error
from autotest_lib.server.cros.faft.firmware_test import FirmwareTest


class firmware_FAFTSetup(FirmwareTest):
    """This test checks the following FAFT hardware requirement:
      - Warm reset
      - Cold reset
      - Recovery boot with USB stick
      - USB stick is plugged into Servo board, not DUT
      - Keyboard simulation
      - No terminal opened on EC console
    """
    version = 1

    # Delay to ensure client is ready to read the key press.
    KEY_PRESS_DELAY = 2


    def console_checker(self):
        """Verify EC console is available if using Chrome EC."""
        if not self.check_ec_capability(suppress_warning=True):
            # Not Chrome EC. Nothing to check.
            return True
        try:
            self.ec.send_command("chan 0")
            expected_output = ["Chip:\s+[^\r\n]*\r\n",
                               "RO:\s+[^\r\n]*\r\n",
                               "RW:\s+[^\r\n]*\r\n",
                               "Build:\s+[^\r\n]*\r\n"]
            self.ec.send_command_get_output("version",
                                            expected_output)
            self.ec.send_command("chan 0xffffffff")
            return True
        except: # pylint: disable=W0702
            logging.error("Cannot talk to EC console.")
            logging.error(
                    "Please check there is no terminal opened on EC console.")
            raise error.TestFail("Failed EC console check.")

    def base_keyboard_checker(self, press_action):
        """Press key and check from DUT.

        Args:
            press_action: A callable that would press the keys when called.
        """
        result = True
        # Stop UI so that key presses don't go to Chrome.
        self.faft_client.system.run_shell_command("stop ui")

        # Press the keys
        Timer(self.KEY_PRESS_DELAY, press_action).start()

        # Invoke client side script to monitor keystrokes
        if not self.faft_client.system.check_keys([28, 29, 32]):
            result = False

        # Turn UI back on
        self.faft_client.system.run_shell_command("start ui")
        return result

    def keyboard_checker(self):
        """Press 'd', Ctrl, ENTER by servo and check from DUT."""

        def keypress():
            self.servo.ctrl_d()
            self.servo.enter_key()

        return self.base_keyboard_checker(keypress)

    def run_once(self):
        logging.info("Check EC console is available and test warm reboot")
        self.console_checker()
        self.switcher.mode_aware_reboot()

        logging.info("Check test image is on USB stick and run recovery boot")
        self.assert_test_image_in_usb_disk()
        self.switcher.reboot_to_mode(to_mode='rec')

        self.check_state((self.checkers.crossystem_checker,
                          {'mainfw_type': 'recovery'}))

        logging.info("Check cold boot")
        self.switcher.mode_aware_reboot(reboot_type='cold')

        if self.faft_config.fw_bypasser_type != 'ctrl_d_bypasser':
            logging.info("Check keyboard simulation")
            self.check_state(self.keyboard_checker)
        else:
            logging.info("Skip keyboard simulation on an embedded device")
