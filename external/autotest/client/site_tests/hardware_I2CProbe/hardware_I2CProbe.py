# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


# DESCRIPTION :
#
# This is a hardware test for Probing I2C device. The test uses i2cdetect
# utility to check if there's an device on specific bus.


import re
import logging

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error


def i2c_detect(bus, addr):
    full_cmd = 'i2cdetect -y %d 0x%x 0x%x' % (bus, addr, addr)
    result = utils.system_output(full_cmd)
    logging.debug('Command: %s', full_cmd)
    logging.debug('Result: %s', result)
    return result

def i2c_probe(bus, addr):
    response = i2c_detect(bus, addr)
    return (re.search('^\d\d:\s+(UU|[0-9a-f]{2})', response, re.MULTILINE) is
            not None)

class hardware_I2CProbe(test.test):
    version = 1

    def run_once(self, bus, addr):
        if not i2c_probe(bus, addr):
            raise error.TestError('No I2C device on bus %d addr 0x%x' %
                    (bus, addr))
