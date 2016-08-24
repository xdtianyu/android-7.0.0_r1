# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, os, glob
from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error

LIGHT_SENSOR_LOCATION = "/sys/bus/iio/devices/*/"
# Match sensors files from
# src/platform2/power_manager/powerd/system/ambient_light_sensor.cc
LIGHT_SENSOR_FILES = [ "in_illuminance0_input",
                       "in_illuminance_input",
                       "in_illuminance0_raw",
                       "in_illuminance_raw",
                       "illuminance0_input",
                     ]

class hardware_LightSensor(test.test):
    """
    Test the system's Light Sensor device.
    Failure to find the device likely indicates the kernel module is not loaded.
    Or it could mean the I2C probe for the device failed because of an incorrect
    I2C address or bus specification.
    The ebuild scripts should properly load the udev rules for light sensor so
    we can find its files in LIGHT_SENSOR_LOCATIONS depending
    on the kernel version.
    """
    version = 1

    def _waiver(self):
        path = os.path.join(self.job.testdir, "hardware_LightSensor",
                            "no_light_sensor_ok")
        if os.path.exists(path):
            return True
        return False


    def run_once(self):
        if self._waiver():
            raise error.TestNAError("Light sensor not required for this device")

        found_light_sensor = 0
        for location in glob.glob(LIGHT_SENSOR_LOCATION):
            for fname in LIGHT_SENSOR_FILES:
                path = location + fname
                if os.path.exists(path):
                    found_light_sensor = 1
                    break
                else:
                    logging.info("Did not find light sensor reading at " + path)

            if found_light_sensor:
                break

        if not found_light_sensor:
            raise error.TestFail("No light sensor reading found.")
        else:
            logging.info("Found light sensor at " + path)

        val = utils.read_one_line(path)
        reading = int(val)
        if reading < 0:
            raise error.TestFail("Invalid light sensor reading (%s)" % val)
        logging.debug("light sensor reading is %d", reading)
