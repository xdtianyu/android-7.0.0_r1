# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import time
from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import power_status, power_utils
from autotest_lib.client.cros import service_stopper
from autotest_lib.client.cros.graphics import graphics_utils


class power_Backlight(test.test):
    version = 1


    def initialize(self):
        """Perform necessary initialization prior to test run.

        Private Attributes:
          _backlight: power_utils.Backlight object
          _services: service_stopper.ServiceStopper object
        """
        super(power_Backlight, self).initialize()
        self._backlight = None
        self._services = service_stopper.ServiceStopper(
            service_stopper.ServiceStopper.POWER_DRAW_SERVICES)
        self._services.stop_services()


    def run_once(self, delay=60, seconds=10, tries=20):
        self._backlight = power_utils.Backlight()


        # disable screen blanking. Stopping screen-locker isn't
        # synchronous :(. Add a sleep for now, till powerd comes around
        # and fixes all this for us.
        # TODO(davidjames): Power manager should support this feature directly
        time.sleep(5)
        graphics_utils.screen_disable_blanking()

        status = power_status.get_status()
        status.assert_battery_state(5)

        max_brightness = self._backlight.get_max_level()
        if max_brightness < 4:
            raise error.TestFail('Must have at least 5 backlight levels')
        sysfs_max = self._get_highest_sysfs_max_brightness()
        if max_brightness != sysfs_max:
            raise error.TestFail(('Max brightness %d is not the highest ' +
                                  'possible |max_brightness|, which is %d') %
                                 (max_brightness, sysfs_max))
        keyvals = {}
        rates = []

        levels = [0, 50, 100]
        for i in levels:
            self._backlight.set_percent(i)
            time.sleep(delay)
            this_rate = []
            for _ in range(tries):
                time.sleep(seconds)
                status.refresh()
                this_rate.append(status.battery[0].energy_rate)
            rate = min(this_rate)
            keyvals['w_bl_%d_rate' % i] = rate
            rates.append(rate)
        self.write_perf_keyval(keyvals)
        for i in range(1, len(levels)):
            if rates[i] <= rates[i-1]:
                raise error.TestFail('Turning up the backlight ' \
                                     'should increase energy consumption')


    def cleanup(self):
        if self._backlight:
            self._backlight.restore()
        self._services.restore_services()
        super(power_Backlight, self).cleanup()


    def _get_highest_sysfs_max_brightness(self):
        # Print |max_brightness| for all backlight sysfs directories, and return
        # the highest of these max_brightness values.
        cmd = 'cat /sys/class/backlight/*/max_brightness'
        output = utils.system_output(cmd)
        return max(map(int, output.split()))
