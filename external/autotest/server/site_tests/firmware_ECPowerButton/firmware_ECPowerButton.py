# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import re
from threading import Timer

from autotest_lib.client.common_lib import error
from autotest_lib.server.cros.faft.firmware_test import FirmwareTest


class firmware_ECPowerButton(FirmwareTest):
    """
    Servo based EC power button test.
    """
    version = 1

    # Delay between shutdown and wake by power button
    LONG_WAKE_DELAY = 13
    SHORT_WAKE_DELAY = 7

    # Short duration of holding down power button to power on
    POWER_BUTTON_SHORT_POWER_ON_DURATION = 0.05

    # Long duration of holding down power button to power on
    POWER_BUTTON_LONG_POWER_ON_DURATION = 1

    # Duration of holding down power button to shut down with powerd
    POWER_BUTTON_POWERD_DURATION = 6

    # Duration of holding down power button to shut down without powerd
    POWER_BUTTON_NO_POWERD_DURATION = 10

    def initialize(self, host, cmdline_args):
        super(firmware_ECPowerButton, self).initialize(host, cmdline_args)
        # Only run in normal mode
        self.switcher.setup_mode('normal')

    def kill_powerd(self):
        """Stop powerd on client."""
        self.faft_client.system.run_shell_command("stop powerd")

    def debounce_power_button(self):
        """Check if power button debouncing works.

        Press power button for a very short period and checks for power
        button keycode.
        """
        # Delay 3 seconds to ensure client machine is waiting for key press.
        # Press power button for only 10ms. Should be debounced.
        logging.info('ECPowerButton: debounce_power_button')
        Timer(3, self.servo.power_key, [0.001]).start()
        return self.faft_client.system.check_keys([116])

    def shutdown_and_wake(self,
                          shutdown_powerkey_duration,
                          wake_delay,
                          wake_powerkey_duration):
        """
        Shutdown the system by power button, delay, and then power on
        by power button again.
        """
        self.servo.power_key(shutdown_powerkey_duration)
        Timer(wake_delay,
              self.servo.power_key,
              [wake_powerkey_duration]).start()

    def run_once(self):
        if not self.check_ec_capability():
            raise error.TestNAError("Nothing needs to be tested on this device")

        logging.info("Shutdown when powerd is still running and wake from S5 "
                     "with short power button press.")

        if self.servo.is_localhost():
            self.check_state(self.debounce_power_button)
        self.switcher.mode_aware_reboot(
                'custom',
                lambda:self.shutdown_and_wake(
                        self.POWER_BUTTON_POWERD_DURATION,
                        self.SHORT_WAKE_DELAY,
                        self.POWER_BUTTON_SHORT_POWER_ON_DURATION))

        logging.info("Shutdown when powerd is stopped and wake from G3 "
                          "with short power button press.")
        self.kill_powerd()
        self.switcher.mode_aware_reboot(
                'custom',
                lambda:self.shutdown_and_wake(
                        self.POWER_BUTTON_NO_POWERD_DURATION,
                        self.LONG_WAKE_DELAY,
                        self.POWER_BUTTON_SHORT_POWER_ON_DURATION))

        logging.info("Shutdown when powerd is still running and wake from G3 "
                     "with long power button press.")
        self.switcher.mode_aware_reboot(
                'custom',
                lambda:self.shutdown_and_wake(
                        self.POWER_BUTTON_POWERD_DURATION,
                        self.LONG_WAKE_DELAY,
                        self.POWER_BUTTON_LONG_POWER_ON_DURATION))

        logging.info("Shutdown when powerd is stopped and wake from S5 "
                     "with long power button press.")
        self.kill_powerd()
        self.switcher.mode_aware_reboot(
                'custom',
                lambda:self.shutdown_and_wake(
                        self.POWER_BUTTON_NO_POWERD_DURATION,
                        self.SHORT_WAKE_DELAY,
                        self.POWER_BUTTON_LONG_POWER_ON_DURATION))
