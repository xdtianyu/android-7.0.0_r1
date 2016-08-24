# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


# DESCRIPTION :
#
# This is a hardware test for system fan.  The test first sets the fan to max
# speed, turns off the fan, and then sets to 50% speed.  The test sleeps for a
# few seconds after setting the fan speed, which allows the fan some time to
# spin up/down and ensures the correctness of the test.  The test restores fan
# setting when finished.  This test uses mosys to read and control fan settings.


import re
import time

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error


class FanControl(object):
    MOSYS_OUTPUT_RE = re.compile('(\w+)="(.*?)"')

    def __init__(self, fan_name='system'):
        self._fan_name = fan_name

    def get_values(self):
        values = {}
        cmd = 'mosys -k sensor print fantach %s' % self._fan_name
        for kv in self.MOSYS_OUTPUT_RE.finditer(utils.system_output(cmd)):
            key, value = kv.groups()
            if key == 'reading':
                value = int(value)
            values[key] = value
        return values

    def get_mode(self):
        return self.get_values()['mode']

    def get_reading(self):
        return self.get_values()['reading']

    def set_percent(self, percent):
        cmd = 'mosys sensor set fantach %s %s' % (self._fan_name, percent)
        utils.system_output(cmd)


class hardware_Fan(test.test):
    version = 1
    DELAY = 3

    def run_once(self):
        fan = FanControl()
        original_values = fan.get_values()
        max_reading = 0

        try:
            fan.set_percent('100')
            time.sleep(self.DELAY)
            if fan.get_mode() != 'manual':
                raise error.TestError('Unable to manually set fan speed')
            if fan.get_reading() == 0:
                raise error.TestError('Fan cannot be turned on')
            max_reading = fan.get_reading()

            fan.set_percent('off')
            time.sleep(self.DELAY)
            if fan.get_reading() != 0:
                raise error.TestError('Unable to turn off fan')

            fan.set_percent('50')
            time.sleep(self.DELAY)
            if not 0 < fan.get_reading() < max_reading:
                raise error.TestError('Fan speed not in reasonable range')
        finally:
            if original_values['mode'] == 'manual' and max_reading > 0:
                fan.set_percent(100 * original_values['reading'] / max_reading)
            else:
                fan.set_percent('auto')
