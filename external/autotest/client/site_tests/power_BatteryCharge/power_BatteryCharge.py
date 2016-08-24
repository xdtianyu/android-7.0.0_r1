# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, time
from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import power_status, power_utils, service_stopper

class power_BatteryCharge(test.test):
    """class power_BatteryCharge."""
    version = 1

    def initialize(self):
        if not power_utils.has_battery():
            raise error.TestNAError('DUT has no battery. Test Skipped')

        self.status = power_status.get_status()

        if not self.status.on_ac():
            raise error.TestNAError(
                  'This test needs to be run with the AC power online')

        self._services = service_stopper.ServiceStopper(
            service_stopper.ServiceStopper.POWER_DRAW_SERVICES + ['ui'])
        self._services.stop_services()


    def run_once(self, max_run_time=180, percent_charge_to_add=1,
                 percent_initial_charge_max=None,
                 percent_target_charge=None,
                 use_design_charge_capacity=True):

        """
        max_run_time: maximum time the test will run for
        percent_charge_to_add: percentage of the charge capacity charge to
                  add. The target charge will be capped at the charge capacity.
        percent_initial_charge_max: maxium allowed initial charge.
        use_design_charge_capacity: If set, use charge_full_design rather than
                  charge_full for calculations. charge_full represents
                  wear-state of battery, vs charge_full_design representing
                  ideal design state.
        """

        time_to_sleep = 60

        self._backlight = power_utils.Backlight()
        self._backlight.set_percent(0)

        self.remaining_time = self.max_run_time = max_run_time

        self.charge_full_design = self.status.battery[0].charge_full_design
        self.charge_full = self.status.battery[0].charge_full
        if use_design_charge_capacity:
            self.charge_capacity = self.charge_full_design
        else:
            self.charge_capacity = self.charge_full

        if self.charge_capacity == 0:
            raise error.TestError('Failed to determine charge capacity')

        self.initial_charge = self.status.battery[0].charge_now
        percent_initial_charge = self.initial_charge * 100 / \
                                 self.charge_capacity
        if percent_initial_charge_max and percent_initial_charge > \
                                          percent_initial_charge_max:
            raise error.TestError('Initial charge (%f) higher than max (%f)'
                      % (percent_initial_charge, percent_initial_charge_max))

        current_charge = self.initial_charge
        if percent_target_charge is None:
            charge_to_add = self.charge_capacity * \
                            float(percent_charge_to_add) / 100
            target_charge = current_charge + charge_to_add
        else:
            target_charge = self.charge_capacity * \
                            float(percent_target_charge) / 100

        # trim target_charge if it exceeds charge capacity
        if target_charge > self.charge_capacity:
            target_charge = self.charge_capacity

        logging.info('max_run_time: %d', self.max_run_time)
        logging.info('initial_charge: %f', self.initial_charge)
        logging.info('target_charge: %f', target_charge)

        while self.remaining_time and current_charge < target_charge:
            if time_to_sleep > self.remaining_time:
                time_to_sleep = self.remaining_time
            self.remaining_time -= time_to_sleep

            time.sleep(time_to_sleep)

            self.status.refresh()
            if not self.status.on_ac():
                raise error.TestError(
                      'This test needs to be run with the AC power online')

            new_charge = self.status.battery[0].charge_now
            logging.info('time_to_sleep: %d', time_to_sleep)
            logging.info('charge_added: %f', (new_charge - current_charge))

            current_charge = new_charge
            logging.info('current_charge: %f', current_charge)

            if self.status.battery[0].status == 'Full':
                logging.info('Battery full, aborting!')
                break


    def postprocess_iteration(self):
        keyvals = {}
        keyvals['ah_charge_full'] = self.charge_full
        keyvals['ah_charge_full_design'] = self.charge_full_design
        keyvals['ah_charge_capacity'] = self.charge_capacity
        keyvals['ah_initial_charge'] = self.initial_charge
        keyvals['ah_final_charge'] = self.status.battery[0].charge_now
        keyvals['s_time_taken'] = self.max_run_time - self.remaining_time
        keyvals['percent_initial_charge'] = self.initial_charge * 100 / \
                                            keyvals['ah_charge_capacity']
        keyvals['percent_final_charge'] = keyvals['ah_final_charge'] * 100 / \
                                          keyvals['ah_charge_capacity']

        percent_charge_added = keyvals['percent_final_charge'] - \
            keyvals['percent_initial_charge']
        # Conditionally write charge current keyval only when the amount of
        # charge added is > 50% to remove samples when test is run but battery
        # is already mostly full.  Otherwise current will be ~0 and not
        # meaningful.
        if percent_charge_added > 50:
            hrs_charging = keyvals['s_time_taken'] / 3600.
            keyvals['a_avg50_charge_current'] = \
                (keyvals['ah_final_charge'] - self.initial_charge) / \
                hrs_charging

        self.write_perf_keyval(keyvals)


    def cleanup(self):
        if hasattr(self, '_services') and self._services:
            self._services.restore_services()
        if hasattr(self, '_backlight') and self._backlight:
            self._backlight.restore()
