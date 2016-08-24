# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


# DESCRIPTION :
#
# This is a hardware test for EC. The test uses ectool to check if the EC can
# receive message from host and send expected reponse back to host. It also
# checks basic EC functionality, such as FAN and temperature sensor.


import time

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import ec as cros_ec


class hardware_EC(test.test):
    """Class for hardware_EC test."""
    version = 1

    def run_once(self,
                 num_temp_sensor=0,
                 temp_sensor_to_test=None,
                 test_fan=False,
                 fan_rpm_error_margin=200,
                 test_battery=False,
                 test_lightbar=False,
                 fan_delay_secs=3):

        ec = cros_ec.EC()

        if not ec.hello():
            raise error.TestError('EC communication failed')

        if test_fan:
            try:
                ec.set_fanspeed(10000)
                time.sleep(fan_delay_secs)
                max_reading = ec.get_fanspeed()
                if max_reading == 0:
                    raise error.TestError('Unable to start fan')

                target_fanspeed = max_reading / 2
                ec.set_fanspeed(target_fanspeed)
                time.sleep(fan_delay_secs)
                current_reading = ec.get_fanspeed()

                # Sometimes the actual fan speed is close but not equal to
                # the target speed, so we add some error margin here.
                lower_bound = target_fanspeed - fan_rpm_error_margin
                upper_bound = target_fanspeed + fan_rpm_error_margin
                if not (lower_bound <= current_reading <= upper_bound):
                    raise error.TestError('Unable to set fan speed')
            finally:
                ec.auto_fan_ctrl()

        if temp_sensor_to_test is None:
            temp_sensor_to_test = list(range(num_temp_sensor))

        for idx in temp_sensor_to_test:
            temperature = ec.get_temperature(idx) - 273
            if temperature < 0 or temperature > 100:
                raise error.TestError(
                        'Abnormal temperature reading on sensor %d' % idx)

        if test_battery and not ec.get_battery():
            raise error.TestError('Battery communication failed')

        if test_lightbar and not ec.get_lightbar():
            raise error.TestError('Lightbar communication failed')
