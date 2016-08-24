# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, time
from autotest_lib.client.bin import test
from autotest_lib.client.bin import utils
from autotest_lib.client.cros import power_status


class power_Draw(test.test):
    version = 1


    def run_once(self, seconds=200, sleep=10):
        status = power_status.get_status()
        if status.on_ac():
            logging.warning('AC power is online -- '
                         'unable to monitor energy consumption')
            return

        # If powerd is running, stop it, so that it cannot interfere with the
        # backlight adjustments in this test.
        if utils.system_output('status powerd').find('start/running') != -1:
            powerd_running = True
            utils.system_output('stop powerd')
        else:
            powerd_running = False

        start_energy = status.battery[0].energy
        self._tlog = power_status.TempLogger([], seconds_period=sleep)
        self._tlog.start()

        # Let the test run
        for i in range(0, seconds, sleep):
            time.sleep(sleep)
            status.refresh()

        status.refresh()
        end_energy = status.battery[0].energy

        consumed_energy = start_energy - end_energy
        energy_rate = consumed_energy * 60 * 60 / seconds

        keyvals = self._tlog.calc()
        keyvals['wh_energy_full'] = status.battery[0].energy_full
        keyvals['wh_start_energy'] = start_energy
        keyvals['wh_end_energy'] = end_energy
        keyvals['wh_consumed_energy'] = consumed_energy
        keyvals['w_average_energy_rate'] = energy_rate
        keyvals['w_end_energy_rate'] = status.battery[0].energy_rate

        self.write_perf_keyval(keyvals)

        # Restore powerd if it was originally running.
        if powerd_running:
            utils.system_output('start powerd');
