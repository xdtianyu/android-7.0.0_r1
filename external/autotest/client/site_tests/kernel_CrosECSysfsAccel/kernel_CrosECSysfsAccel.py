# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


import logging, os
import math
from autotest_lib.client.bin import utils, test
from autotest_lib.client.common_lib import error


class kernel_CrosECSysfsAccel(test.test):
    '''Make sure the EC sysfs accel interface provides meaningful output'''
    version = 1


    # For EC accelerometer, define the number of counts in 1G, and the number
    # of counts that the magnitude of each sensor is allowed to be off from a
    # magnitude of 1G. These values are not sensor dependent, they are based
    # on the EC sysfs interface, which specifies number of counts in 1G.
    _ACCEL_1G = 1024
    _ACCEL_MAG_VALID_OFFSET = 250


    sysfs_accel_search_path = '/sys/bus/iio/devices'
    sysfs_accel_path = ''


    def _read_sysfs_accel_file(self, filename):
        """
        Read the contents of the given accel sysfs file or fail

        @param filename Name of the file within the accel sysfs interface
        directory
        """
        fullpath = os.path.join(self.sysfs_accel_path, filename)

        try:
            content = utils.read_file(fullpath)
        except Exception as err:
            raise error.TestFail('sysfs file problem: %s' % err)
        return content


    def _find_sysfs_accel_dir(self):
        """
        Return the sysfs directory for accessing EC accels
        """
        for _, dirs, _ in os.walk(self.sysfs_accel_search_path):
            for d in dirs:
                namepath = os.path.join(self.sysfs_accel_search_path, d, 'name')

                try:
                    content = utils.read_file(namepath)
                except IOError as err:
                    # errno 2 is code for file does not exist, which is ok
                    # here, just continue on to next directory. Any other
                    # error is a problem, raise an error.
                    if (err.errno == 2):
                        continue
                    raise error.TestFail('IOError %d while searching for accel'
                                         'sysfs dir in %s', err.errno, namepath)

                # Correct directory has a file called 'name' with contents
                # 'cros-ec-accel'
                if content.strip() == 'cros-ec-accel':
                    return os.path.join(self.sysfs_accel_search_path, d)

        raise error.TestFail('No sysfs interface to EC accels (cros-ec-accel)')


    def _verify_accel_data(self, name):
        """
        Verify one of the EC accelerometers through the sysfs interface.
        """
        x = int(self._read_sysfs_accel_file('in_accel_x_' + name + '_raw'))
        y = int(self._read_sysfs_accel_file('in_accel_y_' + name + '_raw'))
        z = int(self._read_sysfs_accel_file('in_accel_z_' + name + '_raw'))
        mag = math.sqrt(x*x + y*y + z*z)

        # Accel data is out of range if magnitude is not close to 1G.
        # Note, this means test will fail on the moon.
        if (abs(mag - self._ACCEL_1G) <= self._ACCEL_MAG_VALID_OFFSET):
            logging.info("%s accel passed. Magnitude is %d.", name, mag)
        else:
            logging.info("%s accel bad data. Magnitude is %d, expected "
                         "%d +/-%d. Raw data is x:%d, y:%d, z:%d.", name,
                         mag, self._ACCEL_1G, self._ACCEL_MAG_VALID_OFFSET,
                         x, y, z)
            raise error.TestFail("Accel magnitude out of range.")


    def run_once(self):
        """
        Check for accelerometers, and if present, check data is valid
        """
        # First make sure that the motion sensors are active. If this
        # check fails it means the EC motion sense task is not running and
        # therefore not updating acceleration values in shared memory.
        active = utils.system_output('ectool motionsense active')
        if active == "0":
            raise error.TestFail("Motion sensing is inactive")

        # Find the iio sysfs directory for EC accels
        self.sysfs_accel_path = self._find_sysfs_accel_dir()
        logging.info("EC accelerometers found at %s", self.sysfs_accel_path)

        # Get all accelerometer data
        accel_info = utils.system_output('ectool motionsense')
        info = accel_info.splitlines()

        # If the base accelerometer is present, then verify data
        if 'None' not in info[1]:
            self._verify_accel_data('base')

        # If the lid accelerometer is present, then verify data
        if 'None' not in info[2]:
            self._verify_accel_data('lid')
