# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


# DESCRIPTION :
#
# Hardware test for temp sensor.  The test uses mosys to read temp sensor value
# and check it's in reasonable range.


import re

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error


# Reasonable temp range for different temp units.
TEMP_RANGE = {
    'degrees C': (0, 100),
}


class TempSensor(object):
    MOSYS_OUTPUT_RE = re.compile('(\w+)="(.*?)"')

    def __init__(self, name):
        self._name = name

    def get_values(self):
        values = {}
        cmd = 'mosys -k sensor print thermal %s' % self._name
        for kv in self.MOSYS_OUTPUT_RE.finditer(utils.system_output(cmd)):
            key, value = kv.groups()
            if key == 'reading':
                value = int(value)
            values[key] = value
        return values

    def get_units(self):
        return self.get_values()['units']

    def get_reading(self):
        return self.get_values()['reading']


class hardware_Thermal(test.test):
    version = 1

    def run_once(self, temp_sensor_names=['temp0']):
        if not temp_sensor_names:
            raise error.TestError('No temp sensor specified')

        for name in temp_sensor_names:
            ts = TempSensor(name)
            units = ts.get_units()
            try:
                low, high = TEMP_RANGE[units]
            except KeyError:
                raise error.TestError('Unknown temp units of %s' % name)
            if not low <= ts.get_reading() <= high:
                raise error.TestError('Temperature of %s out of range' % name)
