# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import time

from autotest_lib.client.common_lib import error
from autotest_lib.server import test

class platform_LongPressPower(test.test):
    """Uses servo pwr_button gpio to power the host off and back on.
    """
    version = 1

    def run_once(self, host):
        boot_id = host.get_boot_id()

        # turn off device
        host.servo.power_long_press()

        # ensure host is now off
        if host.is_up():
            raise error.TestError('DUT still up after long press power')

        # ensure host boots
        host.servo.boot_devmode()
        host.test_wait_for_boot(boot_id)
