# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.common_lib import error
from autotest_lib.server import test

class platform_CloseOpenLid(test.test):
    """Uses servo to send the host to sleep and wake back up.

    Uses pwr_button and lid_open gpios in various combinations.
    """
    version = 1


    def run_once(self, host):
        # lid only
        boot_id = host.get_boot_id()
        host.servo.lid_close()
        host.test_wait_for_shutdown()

        host.servo.lid_open()
        host.servo.pass_devmode()
        host.test_wait_for_boot(boot_id)

        # pwr_button and open lid
        boot_id = host.get_boot_id()
        host.servo.power_long_press()
        if host.is_up():
            raise error.TestError('DUT still up after long press power')

        host.servo.lid_close()
        host.servo.lid_open()
        host.servo.pass_devmode()
        host.test_wait_for_boot(boot_id)
