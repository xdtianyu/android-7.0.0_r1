# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import re
import time
import xmlrpclib

from autotest_lib.client.common_lib import error
from autotest_lib.server.cros.faft.firmware_test import FirmwareTest

class firmware_ECThermal(FirmwareTest):
    """
    Servo based EC thermal engine test.
    """
    version = 1

    # Delay for waiting fan to start or stop
    FAN_DELAY = 5

    # Delay for waiting device stressing to stablize
    STRESS_DELAY = 30

    # Delay for stressing device with fan off to check temperature increase
    STRESS_DELAY_NO_FAN = 12

    # Margin for comparing servo based and ectool based CPU temperature
    TEMP_MISMATCH_MARGIN = 3

    # Minimum increase of CPU temperature when stressing DUT
    TEMP_STRESS_INCREASE = 3

    # Pseudo INT_MAX. Used as infinity when comparing temperature readings
    INT_MAX = 10000

    # Sensor type ID of ignored sensors
    SENSOR_TYPE_IGNORED = 255

    # PID of DUT stressing processes
    _stress_pid = list()

    def enable_auto_fan_control(self):
        """Enable EC automatic fan speed control"""
        # We use set_nocheck because servo reports current target
        # RPM instead 'auto', and therefore servo.set always fails.
        self.servo.set_nocheck('fan_target_rpm', 'auto')


    def max_fan(self):
        """Maximize fan speed"""
        # We use set_nocheck because servo reports current target
        # RPM instead 'max', and therefore servo.set always fails.
        self.servo.set_nocheck('fan_target_rpm', 'max')


    def turn_off_fan(self):
        """Turn off fan"""
        self.servo.set('fan_target_rpm', 'off')


    def _get_setting_for_type(self, type_id):
        """
        Retrieve thermal setting for a given type of sensor

        Args:
          type_id: The ID of sensor type.

        Returns:
          A list containing thresholds in the following order:
            Warning
            CPU off
            All power off
            Fan speed thresholds
        """
        setting = list()
        current_id = 0
        while True:
            try:
                lines = self.faft_client.system.run_shell_command_get_output(
                        'ectool thermalget %d %d' % (type_id, current_id))
            except xmlrpclib.Fault:
                break
            pattern = re.compile('Threshold \d* [a-z ]* \d* is (\d*) K.')
            for line in lines:
                matched = pattern.match(line)
                if matched is not None:
                    # Convert degree K to degree C
                    setting.append(int(matched.group(1)) - 273)
            current_id = current_id + 1

        if len(setting) == 0:
            return None
        return setting


    def get_fan_steps(self):
        """Retrieve fan step config from EC"""
        num_steps = len(self._thermal_setting[0]) - 3
        self._fan_steps = list()
        expected_pat = (["Lowest speed: ([0-9-]+) RPM"] +
                        ["\d+ K:\s+([0-9-]+) RPM"] * num_steps)
        match = self.ec.send_command_get_output("thermalfan 0", expected_pat)
        for m in match:
            self._fan_steps.append(int(m[1]))

        # Get the actual value of each fan step
        for i in xrange(num_steps + 1):
            if self._fan_steps[i] == 0:
                continue
            self.servo.set_nocheck('fan_target_rpm', "%d" % self._fan_steps[i])
            self._fan_steps[i] = int(self.servo.get('fan_target_rpm'))

        logging.info("Actual fan steps: %s", self._fan_steps)


    def get_thermal_setting(self):
        """Retrieve thermal engine setting from EC"""
        self._thermal_setting = list()
        type_id = 0
        while True:
            setting = self._get_setting_for_type(type_id)
            if setting is None:
                break
            self._thermal_setting.append(setting)
            type_id = type_id + 1
        logging.info("Number of tempearture sensor types: %d", type_id)

        # Get the number of temperature sensors
        self._num_temp_sensor = 0
        while True:
            try:
                self.faft_client.system.run_shell_command('ectool temps %d' %
                                                   self._num_temp_sensor)
                self._num_temp_sensor = self._num_temp_sensor + 1
            except xmlrpclib.Fault:
                break
        logging.info("Number of temperature sensor: %d", self._num_temp_sensor)


    def initialize(self, host, cmdline_args):
        super(firmware_ECThermal, self).initialize(host, cmdline_args)
        self.ec.send_command("chan 0")
        try:
            self.faft_client.system.run_shell_command('stop temp_metrics')
        except xmlrpclib.Fault:
            self._has_temp_metrics = False
        else:
            logging.info('Stopped temp_metrics')
            self._has_temp_metrics = True
        if self.check_ec_capability(['thermal']):
            self.get_thermal_setting()
            self.get_fan_steps()
            self.enable_auto_fan_control()


    def cleanup(self):
        if self.check_ec_capability(['thermal']):
            self.enable_auto_fan_control()
        if self._has_temp_metrics:
            logging.info('Starting temp_metrics')
            self.faft_client.system.run_shell_command('start temp_metrics')
        self.ec.send_command("chan 0xffffffff")
        super(firmware_ECThermal, self).cleanup()


    def _find_cpu_sensor_id(self):
        """
        This function find CPU temperature sensor using ectool.

        Returns:
          Integer ID of CPU temperature sensor.

        Raises:
          error.TestFail: Raised if we fail to find PECI temparture through
            ectool.
        """
        for temp_id in range(self._num_temp_sensor):
            lines = self.faft_client.system.run_shell_command_get_output(
                    'ectool tempsinfo %d' % temp_id)
            for line in lines:
                matched = re.match('Sensor name: (.*)', line)
                if matched is not None and matched.group(1) == 'PECI':
                    return temp_id
        raise error.TestFail('Cannot find CPU temperature sensor ID.')


    def _get_temp_reading(self, sensor_id):
        """
        Get temperature reading on a sensor through ectool

        Args:
          sensor_id: Temperature sensor ID.

        Returns:
          Temperature reading in degree C.

        Raises:
          xmlrpclib.Fault: Raised when we fail to read temperature.
          error.TestError: Raised if ectool doesn't behave as we expected.
        """
        assert sensor_id < self._num_temp_sensor
        pattern = re.compile('Reading temperature...(\d*)')
        lines = self.faft_client.system.run_shell_command_get_output(
                'ectool temps %d' % sensor_id)
        for line in lines:
            matched = pattern.match(line)
            if matched is not None:
                return int(matched.group(1)) - 273
        # Should never reach here
        raise error.TestError("Unexpected error occurred")


    def check_temp_report(self):
        """
        Checker of temperature reporting.

        This function reads CPU temperature from servo and ectool. If
        the two readings mismatches by more than TEMP_MISMATCH_MARGIN,'
        test fails.

        Raises:
          error.TestFail: Raised when temperature reading mismatches by
            more than TEMP_MISMATCH_MARGIN.
        """
        cpu_temp_id = self._find_cpu_sensor_id()
        logging.info("CPU temperature sensor ID is %d", cpu_temp_id)
        ectool_cpu_temp = self._get_temp_reading(cpu_temp_id)
        servo_cpu_temp = int(self.servo.get('cpu_temp'))
        logging.info("CPU temperature from servo: %d C", servo_cpu_temp)
        logging.info("CPU temperature from ectool: %d C", ectool_cpu_temp)
        if abs(ectool_cpu_temp - servo_cpu_temp) > self.TEMP_MISMATCH_MARGIN:
            raise error.TestFail(
                    'CPU temperature readings from servo and ectool differ')


    def _stress_dut(self, threads=4):
        """
        Stress DUT system.

        By reading from /dev/urandom and writing to /dev/null, we can stress
        DUT and cause CPU temperature to go up. We stress the system forever,
        until _stop_stressing is called to kill the stress threads. This
        function is non-blocking.

        Args:
          threads: Number of threads (processes) when stressing forever.

        Returns:
          A list of stress process IDs is returned.
        """
        logging.info("Stressing DUT with %d threads...", threads)
        self.faft_client.system.run_shell_command('pkill dd')
        stress_cmd = 'dd if=/dev/urandom of=/dev/null bs=1M &'
        # Grep for [d]d instead of dd to prevent getting the PID of grep
        # itself.
        pid_cmd = "ps -ef | grep '[d]d if=/dev/urandom' | awk '{print $2}'"
        self._stress_pid = list()
        for _ in xrange(threads):
            self.faft_client.system.run_shell_command(stress_cmd)
        lines = self.faft_client.system.run_shell_command_get_output(
                    pid_cmd)
        for line in lines:
            logging.info("PID is %s", line)
            self._stress_pid.append(int(line.strip()))
        return self._stress_pid


    def _stop_stressing(self):
        """Stop stressing DUT system"""
        stop_cmd = 'kill -9 %d'
        for pid in self._stress_pid:
            self.faft_client.system.run_shell_command(stop_cmd % pid)


    def check_fan_off(self):
        """
        Checker of fan turned off.

        The function first delay FAN_DELAY seconds to ensure fan stops.
        Then it reads fan speed and return False if fan speed is non-zero.
        Then it stresses the system a bit and check if the temperature
        goes up by more than TEMP_STRESS_INCREASE.

        Raises:
          error.TestFail: Raised when temperature doesn't increase by more than
            TEMP_STRESS_INCREASE.
        """
        time.sleep(self.FAN_DELAY)
        fan_speed = self.servo.get('fan_actual_rpm')
        if int(fan_speed) != 0:
            raise error.TestFail("Fan is not turned off.")
        logging.info("EC reports fan turned off.")
        cpu_temp_before = int(self.servo.get('cpu_temp'))
        logging.info("CPU temperature before stressing is %d C",
                     cpu_temp_before)
        self._stress_dut()
        time.sleep(self.STRESS_DELAY_NO_FAN)
        cpu_temp_after = int(self.servo.get('cpu_temp'))
        self._stop_stressing()
        logging.info("CPU temperature after stressing is %d C",
                     cpu_temp_after)
        if cpu_temp_after - cpu_temp_before < self.TEMP_STRESS_INCREASE:
            raise error.TestFail(
                    "CPU temperature did not go up by more than %d degrees" %
                    self.TEMP_STRESS_INCREASE)


    def _get_temp_sensor_type(self, sensor_id):
        """
        Get type of a given temperature sensor

        Args:
          sensor_id: Temperature sensor ID.

        Returns:
          Type ID of the temperature sensor.

        Raises:
          error.TestError: Raised when ectool doesn't behave as we expected.
        """
        assert sensor_id < self._num_temp_sensor
        pattern = re.compile('Sensor type: (\d*)')
        lines = self.faft_client.system.run_shell_command_get_output(
                'ectool tempsinfo %d' % sensor_id)
        for line in lines:
            matched = pattern.match(line)
            if matched is not None:
                return int(matched.group(1))
        # Should never reach here
        raise error.TestError("Unexpected error occurred")


    def _check_fan_speed_per_sensor(self, fan_speed, sensor_id):
        """
        Check if the given fan_speed is reasonable from the view of certain
        temperature sensor. There could be three types of outcome:
          1. Fan speed is higher than expected. This may be due to other
             sensor sensing higher temperature and setting fan to higher
             speed.
          2. Fan speed is as expected.
          3. Fan speed is lower than expected. In this case, EC is not
             working as expected and an error should be raised.

        Args:
          fan_speed: The current fan speed in RPM.
          sensor_id: The ID of temperature sensor.

        Returns:
          0x00: Fan speed is higher than expected.
          0x01: Fan speed is as expected.
          0x10: Fan speed is lower than expected.

        Raises:
          error.TestError: Raised when getting unexpected fan speed.
        """
        sensor_type = self._get_temp_sensor_type(sensor_id)
        if sensor_type == self.SENSOR_TYPE_IGNORED:
            # This sensor should be ignored
            return 0x00

        if self._thermal_setting[sensor_type][-1] == -273:
            # The fan stepping for this type of sensor is disabled
            return 0x00

        try:
            idx = self._fan_steps.index(fan_speed)
        except:
            raise error.TestError("Unexpected fan speed: %d" % fan_speed)

        if idx == 0:
            lower_bound = -self.INT_MAX
            upper_bound = self._thermal_setting[sensor_type][3]
        elif idx == len(self._fan_steps) - 1:
            lower_bound = self._thermal_setting[sensor_type][idx + 2] - 3
            upper_bound = self.INT_MAX
        else:
            lower_bound = self._thermal_setting[sensor_type][idx + 2] - 3
            upper_bound = self._thermal_setting[sensor_type][idx + 3]

        temp_reading = self._get_temp_reading(sensor_id)
        logging.info("Sensor %d = %d C", sensor_id, temp_reading)
        logging.info("  Expecting %d - %d C", lower_bound, upper_bound)
        if temp_reading > upper_bound:
            return 0x00
        elif temp_reading < lower_bound:
            return 0x10
        else:
            return 0x01


    def check_auto_fan(self):
        """
        Checker of thermal engine automatic fan speed control.

        Stress DUT system for a longer period to make temperature more stable
        and check if fan speed is controlled as expected.

        Raises:
          error.TestFail: Raised when fan speed is not as expected.
        """
        self._stress_dut()
        time.sleep(self.STRESS_DELAY)
        fan_rpm = int(self.servo.get('fan_target_rpm'))
        logging.info('Fan speed is %d RPM', fan_rpm)
        try:
            result = reduce(lambda x, y: x | y,
                            [self._check_fan_speed_per_sensor(fan_rpm, x)
                             for x in range(self._num_temp_sensor)])
        finally:
            self._stop_stressing()
        if result == 0x00:
            raise error.TestFail("Fan speed higher than expected")
        if result == 0x10:
            raise error.TestFail("Fan speed lower than expected")


    def run_once(self):
        if not self.check_ec_capability(['thermal']):
            raise error.TestNAError("Nothing needs to be tested on this device")
        logging.info("Checking host temperature report.")
        self.check_temp_report()

        self.turn_off_fan()
        logging.info("Verifying fan is turned off.")
        self.check_fan_off()

        self.enable_auto_fan_control()
        logging.info("Verifying automatic fan control functionality.")
        self.check_auto_fan()
