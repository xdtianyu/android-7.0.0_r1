# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import re

from autotest_lib.client.common_lib import error
from autotest_lib.server.cros.faft.firmware_test import FirmwareTest

class firmware_ECBattery(FirmwareTest):
    """
    Servo based EC thermal battery status report test.
    """
    version = 1

    # Battery status path in sysfs
    BATTERY_STATUS = '/sys/class/power_supply/%s/status'

    # Battery voltage reading path in sysfs
    BATTERY_VOLTAGE_READING = '/sys/class/power_supply/%s/voltage_now'

    # Battery current reading path in sysfs
    BATTERY_CURRENT_READING = '/sys/class/power_supply/%s/current_now'

    # Maximum allowed error of voltage reading in mV
    VOLTAGE_MV_ERROR_MARGIN = 300

    # Maximum allowed error of current reading in mA
    CURRENT_MA_ERROR_MARGIN = 300

    # Maximum allowed battery temperature in C
    BATTERY_TEMP_UPPER_BOUND = 70

    # Minimum allowed battery temperature in C
    BATTERY_TEMP_LOWER_BOUND = 0


    def initialize(self, host, cmdline_args):
        super(firmware_ECBattery, self).initialize(host, cmdline_args)
        # Only run in normal mode
        self.switcher.setup_mode('normal')
        self.ec.send_command("chan 0")


    def cleanup(self):
        self.ec.send_command("chan 0xffffffff")
        super(firmware_ECBattery, self).cleanup()


    def _get_battery_path(self):
        """Get battery path in sysfs."""
        match = self.faft_client.system.run_shell_command_get_output(
                'grep -iH --color=no "Battery" /sys/class/power_supply/*/type')
        name = re.search("/sys/class/power_supply/([^/]+)/",
                         match[0]).group(1)
        logging.info("Battery name is %s", name)
        self._battery_status = self.BATTERY_STATUS % name
        self._battery_voltage = self.BATTERY_VOLTAGE_READING % name
        self._battery_current = self.BATTERY_CURRENT_READING % name


    def _check_voltage_match(self):
        """Check if voltage reading from kernel and servo match.

        Raises:
          error.TestFail: Raised when the two reading mismatch by more than
            VOLTAGE_MV_ERROR_MARGIN mV.
        """
        servo_reading = int(self.servo.get('ppvar_vbat_mv'))
        # Kernel gives voltage value in uV. Convert to mV here.
        kernel_reading = int(
            self.faft_client.system.run_shell_command_get_output(
                'cat %s' % self._battery_voltage)[0]) / 1000
        logging.info("Voltage reading from servo: %dmV", servo_reading)
        logging.info("Voltage reading from kernel: %dmV", kernel_reading)
        if abs(servo_reading - kernel_reading) > self.VOLTAGE_MV_ERROR_MARGIN:
            raise error.TestFail(
                    "Voltage reading from servo (%dmV) and kernel (%dmV) "
                    "mismatch." % (servo_reading, kernel_reading))


    def _check_current_match(self):
        """Check if current reading from kernel and servo match.

        Raises:
          error.TestFail: Raised when the two reading mismatch by more than
            CURRENT_MA_ERROR_MARGIN mA.
        """
        # The signs of the current values from servo and kernel are not
        # consistent across different devices. So we pick the absolute values.
        # TODO(victoryang@chromium.org): Investigate the sign issue.
        servo_reading = abs(int(self.servo.get('ppvar_vbat_ma')))
        # Kernel gives current value in uA. Convert to mA here.
        kernel_reading = abs(
            int(self.faft_client.system.run_shell_command_get_output(
                'cat %s' % self._battery_current)[0])) / 1000
        logging.info("Current reading from servo: %dmA", servo_reading)
        logging.info("Current reading from kernel: %dmA", kernel_reading)
        if abs(servo_reading - kernel_reading) > self.CURRENT_MA_ERROR_MARGIN:
            raise error.TestFail(
                    "Current reading from servo (%dmA) and kernel (%dmA) "
                    "mismatch." % (servo_reading, kernel_reading))


    def _check_temperature(self):
        """Check if battery temperature is reasonable.

        Raises:
          error.TestFail: Raised when battery tempearture is higher than
            BATTERY_TEMP_UPPER_BOUND or lower than BATTERY_TEMP_LOWER_BOUND.
        """
        battery_temp = float(self.ec.send_command_get_output("battery",
                ["Temp:.+\(([0-9\.]+) C\)"])[0][1])
        logging.info("Battery temperature is %f C", battery_temp)
        if (battery_temp > self.BATTERY_TEMP_UPPER_BOUND or
            battery_temp < self.BATTERY_TEMP_LOWER_BOUND):
            raise error.TestFail("Abnormal battery temperature, %.2f C." %
                                 battery_temp)


    def run_once(self):
        if not self.check_ec_capability(['battery']):
            raise error.TestNAError("Nothing needs to be tested on this device")

        self._get_battery_path()

        logging.info("Checking battery current reading...")
        self._check_current_match()

        logging.info("Checking battery voltage reading...")
        self._check_voltage_match()

        logging.info("Checking battery temperature...")
        self._check_temperature()
