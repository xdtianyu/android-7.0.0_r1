# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, math, time

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import power_status, rtc, sys_power

class power_Standby(test.test):
    """Measure Standby(S3) power test."""
    version = 1
    _percent_min_charge = 0.1
    _min_sample_hours = 0.1

    def run_once(self, test_hours=None, sample_hours=None,
                 percent_initial_charge_min=0.2, max_milliwatts_standby=None):

        if test_hours <= sample_hours:
            raise error.TestFail("Test hours must be greater than sample hours")

        # If we're measuring <= 6min of S3 then the S0 time is not negligible.
        # Note, reasonable rule of thumb is S0 idle is ~10-20 times S3 power.
        if sample_hours < self._min_sample_hours:
            raise error.TestFail("Must suspend more than %.2f hours" % \
                                 sample_hours)

        # Query initial power status
        power_stats = power_status.get_status()
        power_stats.assert_battery_state(percent_initial_charge_min)
        charge_start = power_stats.battery[0].charge_now
        voltage_start = power_stats.battery[0].voltage_now

        max_hours = charge_start * voltage_start / \
            (max_milliwatts_standby / 1000)
        if max_hours < test_hours:
            raise error.TestFail('Battery not charged adequately for test')

        elapsed_hours = 0

        while elapsed_hours < test_hours:
            charge_before = power_stats.battery[0].charge_now
            before_suspend_secs = rtc.get_seconds()
            sys_power.do_suspend(sample_hours * 3600)
            after_suspend_secs = rtc.get_seconds()

            power_stats.refresh()
            if power_stats.percent_current_charge() < self._percent_min_charge:
                logging.warning("Battery percent = %.2f%%.  Too low to continue")
                break

            # check that the RTC slept the correct amount of time as there could
            # potentially be another wake source that would spoil the test.
            actual_hours = (after_suspend_secs - before_suspend_secs) / 3600.0
            logging.debug("actual_hours = %.4f", actual_hours)
            percent_diff = math.fabs((actual_hours - sample_hours) /
                                     ((actual_hours + sample_hours) / 2) * 100)
            if percent_diff > 2:
                err_str = "Requested S3 time and actual varied by %.2f%%." \
                    % percent_diff
                raise error.TestFail(err_str)

            # Check resulting charge consumption
            charge_used = charge_before - power_stats.battery[0].charge_now
            logging.debug("charge_used = %.6f", charge_used)

            elapsed_hours += actual_hours
            logging.debug("elapsed_hours = %.4f", elapsed_hours)

        charge_end = power_stats.battery[0].charge_now
        voltage_end = power_stats.battery[0].voltage_now
        standby_hours = power_stats.battery[0].charge_full_design / \
            (charge_start - charge_end) * elapsed_hours
        energy_used = charge_start * voltage_start - charge_end * voltage_end
        if energy_used <= 0:
            raise error.TestError("Energy used reading is suspect.")
        standby_milliwatts = energy_used / elapsed_hours * 1000

        results = {}
        results['milliwatts_standby_power'] = standby_milliwatts
        results['hours_standby_time'] = standby_hours
        self.write_perf_keyval(results)

        # need to sleep for some time to allow network connection to return
        time.sleep(10)
