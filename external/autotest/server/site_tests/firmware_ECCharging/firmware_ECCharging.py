# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.client.common_lib import error
from autotest_lib.server.cros.faft.firmware_test import FirmwareTest

class firmware_ECCharging(FirmwareTest):
    """
    Servo based EC charging control test.
    """
    version = 1

    # Threshold of trickle charging current in mA
    TRICKLE_CHARGE_THRESHOLD = 100

    def initialize(self, host, cmdline_args):
        super(firmware_ECCharging, self).initialize(host, cmdline_args)
        # Only run in normal mode
        self.switcher.setup_mode('normal')
        self.ec.send_command("chan 0")


    def cleanup(self):
        self.ec.send_command("chan 0xffffffff")
        super(firmware_ECCharging, self).cleanup()


    def _get_battery_desired_voltage(self):
        """Get battery desired voltage value."""
        voltage = int(self.ec.send_command_get_output("battery",
                ["V-desired:\s+0x[0-9a-f]*\s+=\s+(\d+)\s+mV"])[0][1])
        logging.info("Battery desired voltage = %d mV", voltage)
        return voltage


    def _get_battery_desired_current(self):
        """Get battery desired current value."""
        current = int(self.ec.send_command_get_output("battery",
                ["I-desired:\s+0x[0-9a-f]*\s+=\s+(\d+)\s+mA"])[0][1])
        logging.info("Battery desired current = %d mA", current)
        return current


    def _get_battery_actual_voltage(self):
        """Get the actual voltage from charger to battery."""
        voltage = int(self.ec.send_command_get_output("battery",
                ["V:\s+0x[0-9a-f]*\s+=\s+(\d+)\s+mV"])[0][1])
        logging.info("Battery actual voltage = %d mV", voltage)
        return voltage


    def _get_battery_actual_current(self):
        """Get the actual current from charger to battery."""
        current = int(self.ec.send_command_get_output("battery",
                ["I:\s+0x[0-9a-f]*\s+=\s+([0-9-]+)\s+mA"])[0][1])
        logging.info("Battery actual current = %d mA", current)
        return current


    def _get_battery_charge(self):
        """Get battery charge state."""
        charge = int(self.ec.send_command_get_output("battery",
                ["Charge:\s+(\d+)\s+"])[0][1])
        logging.info("Battery charge = %d %%", charge)
        return charge


    def _get_charger_target_voltage(self):
        """Get target charging voltage set in charger."""
        voltage = int(self.ec.send_command_get_output("charger",
                ["V_batt:\s+(\d+)\s"])[0][1])
        logging.info("Charger target voltage = %d mV", voltage)
        return voltage


    def _get_charger_target_current(self):
        """Get target charging current set in charger."""
        current = int(self.ec.send_command_get_output("charger",
                ["I_batt:\s+(\d+)\s"])[0][1])
        logging.info("Charger target current = %d mA", current)
        return current


    def _get_trickle_charging(self):
        """Check if we are trickle charging battery."""
        return (self._get_battery_desired_current() <
                self.TRICKLE_CHARGE_THRESHOLD)


    def _check_target_value(self):
        """Check charger target values are correct.

        Raise:
          error.TestFail: Raised when check fails.
        """
        if (self._get_charger_target_voltage() >=
                1.05 * self._get_battery_desired_voltage()):
            raise error.TestFail("Charger target voltage is too high.")
        if (self._get_charger_target_current() >=
                1.05 * self._get_battery_desired_current()):
            raise error.TestFail("Charger target current is too high.")


    def _check_actual_value(self):
        """Check actual voltage/current values are correct.

        Raise:
          error.TestFail: Raised when check fails.
        """
        if (self._get_battery_actual_voltage() >=
                1.05 * self._get_charger_target_voltage()):
            raise error.TestFail("Battery actual voltage is too high.")
        if (self._get_battery_actual_current() >=
                1.05 * self._get_charger_target_current()):
            raise error.TestFail("Battery actual current is too high.")


    def run_once(self):
        if not self.check_ec_capability(['battery', 'charging']):
            raise error.TestNAError("Nothing needs to be tested on this device")
        if self._get_battery_charge() == 100:
            logging.info("Battery is full. Unable to test.")
            return
        if self._get_trickle_charging():
            logging.info("Trickling charging battery. Unable to test.")
            return
        if self._get_battery_actual_current() < 0:
            raise error.TestFail("This test must be run with AC power.")

        logging.info("Checking charger target values...")
        self._check_target_value()

        logging.info("Checking battery actual values...")
        self._check_actual_value()
