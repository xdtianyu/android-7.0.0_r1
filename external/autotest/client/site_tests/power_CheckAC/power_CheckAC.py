# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import power_status

class power_CheckAC(test.test):
    """Check the line status for AC power

    This is meant for verifying system setups in the lab to make sure that the
    AC line status can be remotely turned on and off.
    """
    version = 1


    def run_once(self, power_on=True):
        status = power_status.get_status()
        if power_on and not status.on_ac():
            raise error.TestError('AC line status is not on but should be')
        elif not power_on and status.on_ac():
            raise error.TestError('AC line status is on but should not be')
