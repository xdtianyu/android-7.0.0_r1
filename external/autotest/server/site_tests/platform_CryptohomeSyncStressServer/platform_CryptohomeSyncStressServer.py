# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import random, time
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import power_strip
from autotest_lib.server import autotest, test

class platform_CryptohomeSyncStressServer(test.test):
    version = 1
    max_delay = 120

    def run_once(self, host, power_addr, outlet, username, password):
        # check parameters
        if power_addr == None:
            raise error.TestFail('Missing power_addr argument.')
        if outlet == None:
            raise error.TestFail('Missing outlet argument.')
        if username == None:
            raise error.TestFail('Missing user parameter.')
        if password == None:
            raise error.TestFail('Missing pass parameter.')

        outlet = int(outlet)

        at = autotest.Autotest(host)
        boot_id = host.get_boot_id()

        # log in and verify things work
        self.job.set_state('client_fail', True)
        at.run_test('platform_CryptohomeSyncStress',
                    username=username, password=password)
        if self.job.get_state('client_fail'):
            raise error.TestFail('Client test failed')

        # wait for some delay
        delay = random.randint(0, self.max_delay)
        print 'Delaying for %s seconds and then restarting.' % (delay)
        time.sleep(delay)

        # restart client
        power_strip.PowerStrip(power_addr).reboot(outlet)
        host.wait_for_restart(old_boot_id=boot_id)
