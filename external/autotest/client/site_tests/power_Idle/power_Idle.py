# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, time

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib.cros import chrome
from autotest_lib.client.cros import power_rapl, power_status, power_utils
from autotest_lib.client.cros import service_stopper


class power_Idle(test.test):
    version = 1

    def initialize(self):
        """Perform necessary initialization prior to test run.

        Private Attributes:
          _backlight: power_utils.Backlight object
          _services: service_stopper.ServiceStopper object
        """
        super(power_Idle, self).initialize()
        self._backlight = None
        self._services = service_stopper.ServiceStopper(
            service_stopper.ServiceStopper.POWER_DRAW_SERVICES)
        self._services.stop_services()


    def warmup(self, warmup_time=60):
        time.sleep(warmup_time)


    def run_once(self, idle_time=120, sleep=10):
        with chrome.Chrome():
            self._backlight = power_utils.Backlight()
            self._backlight.set_default()

            self._start_time = time.time()
            self.status = power_status.get_status()
            self._stats = power_status.StatoMatic()

            measurements = []
            if not self.status.on_ac():
                measurements.append(
                    power_status.SystemPower(self.status.battery_path))
            if power_utils.has_rapl_support():
                measurements += power_rapl.create_rapl()
            self._plog = power_status.PowerLogger(measurements,
                                                  seconds_period=sleep)
            self._tlog = power_status.TempLogger([], seconds_period=sleep)
            self._plog.start()
            self._tlog.start()

            for _ in xrange(0, idle_time, sleep):
                time.sleep(sleep)
                self.status.refresh()
            self.status.refresh()
            self._plog.checkpoint('', self._start_time)
            self._tlog.checkpoint('', self._start_time)


    def postprocess_iteration(self):
        keyvals = self._stats.publish()

        # record the current and max backlight levels
        self._backlight = power_utils.Backlight()
        keyvals['level_backlight_max'] = self._backlight.get_max_level()
        keyvals['level_backlight_current'] = self._backlight.get_level()

        # record battery stats if not on AC
        if self.status.on_ac():
            keyvals['b_on_ac'] = 1
        else:
            keyvals['b_on_ac'] = 0
            keyvals['ah_charge_full'] = self.status.battery[0].charge_full
            keyvals['ah_charge_full_design'] = \
                                self.status.battery[0].charge_full_design
            keyvals['ah_charge_now'] = self.status.battery[0].charge_now
            keyvals['a_current_now'] = self.status.battery[0].current_now
            keyvals['wh_energy'] = self.status.battery[0].energy
            keyvals['w_energy_rate'] = self.status.battery[0].energy_rate
            keyvals['h_remaining_time'] = self.status.battery[0].remaining_time
            keyvals['v_voltage_min_design'] = \
                                self.status.battery[0].voltage_min_design
            keyvals['v_voltage_now'] = self.status.battery[0].voltage_now
            keyvals.update(self._plog.calc())

        keyvals.update(self._tlog.calc())
        logging.debug("keyvals = %s", keyvals)

        self.write_perf_keyval(keyvals)


    def cleanup(self):
        if self._backlight:
            self._backlight.restore()
        self._services.restore_services()
        super(power_Idle, self).cleanup()
