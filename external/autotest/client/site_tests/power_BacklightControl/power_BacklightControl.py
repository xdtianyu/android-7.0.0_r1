# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, time
from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import power_status, power_utils
from autotest_lib.client.cros.graphics import graphics_utils


def get_num_outputs_on():
    """
    Retrieves the number of connected outputs that are on.
    @return: integer value of number of connected outputs that are on.
    """

    return graphics_utils.get_num_outputs_on();

class power_BacklightControl(test.test):
    version = 1
    # Minimum number of steps expected between min and max brightness levels.
    _min_num_steps = 4
    # Minimum required percentage change in energy rate between transitions
    # (max -> min, min-> off)
    _energy_rate_change_threshold_percent = 5


    def initialize(self):
        """Perform necessary initialization prior to test run.

        Private Attributes:
          _backlight: power_utils.Backlight object
        """
        super(power_BacklightControl, self).initialize()
        self._backlight = None


    def run_once(self):
        # Require that this test be run on battery with at least 5% charge
        status = power_status.get_status()
        status.assert_battery_state(5)

        prefs = { 'has_ambient_light_sensor' : 0,
                  'ignore_external_policy'   : 1,
                  'plugged_dim_ms'           : 7200000,
                  'plugged_off_ms'           : 9000000,
                  'plugged_suspend_ms'       : 18000000,
                  'unplugged_dim_ms'         : 7200000,
                  'unplugged_off_ms'         : 9000000,
                  'unplugged_suspend_ms'     : 18000000 }
        self._pref_change = power_utils.PowerPrefChanger(prefs)

        keyvals = {}
        num_errors = 0

        # These are the expected ratios of energy rate between max, min, and off
        # (zero) brightness levels.  e.g. when changing from max to min, the
        # energy rate must become <= (max_energy_rate * max_to_min_factor).
        max_to_min_factor = \
            1.0 - self._energy_rate_change_threshold_percent / 100.0
        min_to_off_factor = \
            1.0 - self._energy_rate_change_threshold_percent / 100.0
        off_to_max_factor = 1.0 / (max_to_min_factor * min_to_off_factor)

        # Determine the number of outputs that are on.
        starting_num_outputs_on = get_num_outputs_on()
        if starting_num_outputs_on == 0:
            raise error.TestFail('At least one display output must be on.')
        keyvals['starting_num_outputs_on'] = starting_num_outputs_on

        self._backlight = power_utils.Backlight()
        keyvals['max_brightness'] = self._backlight.get_max_level()
        if keyvals['max_brightness'] <= self._min_num_steps:
            raise error.TestFail('Must have at least %d backlight levels' %
                                 (self._min_num_steps + 1))

        keyvals['initial_brightness'] = self._backlight.get_level()

        self._wait_for_stable_energy_rate()
        keyvals['initial_power_w'] = self._get_current_energy_rate()

        self._backlight_controller = power_utils.BacklightController()
        self._backlight_controller.set_brightness_to_max()

        current_brightness = \
            utils.wait_for_value(self._backlight.get_level,
                                 max_threshold=keyvals['max_brightness'])
        if current_brightness != keyvals['max_brightness']:
            num_errors += 1
            logging.error(('Failed to increase brightness to max, ' + \
                           'brightness is %d.') % current_brightness)
        else:
            self._wait_for_stable_energy_rate()
            keyvals['max_brightness_power_w'] = self._get_current_energy_rate()

        # Set brightness to minimum without going to zero.
        # Note that we don't know what the minimum brightness is, so just set
        # min_threshold=0 to use the timeout to wait for the brightness to
        # settle.
        self._backlight_controller.set_brightness_to_min()
        current_brightness = utils.wait_for_value(
            self._backlight.get_level,
            min_threshold=(keyvals['max_brightness'] / 2 - 1))
        if current_brightness >= keyvals['max_brightness'] / 2 or \
           current_brightness == 0:
            num_errors += 1
            logging.error('Brightness is not at minimum non-zero level: %d' %
                          current_brightness)
        else:
            self._wait_for_stable_energy_rate()
            keyvals['min_brightness_power_w'] = self._get_current_energy_rate()

        # Turn off the screen by decreasing brightness one more time with
        # allow_off=True.
        self._backlight_controller.decrease_brightness(True)
        current_brightness = utils.wait_for_value(
            self._backlight.get_level, min_threshold=0)
        if current_brightness != 0:
            num_errors += 1
            logging.error('Brightness is %d, expecting 0.' % current_brightness)

        # Wait for screen to turn off.
        num_outputs_on = utils.wait_for_value(
            get_num_outputs_on, min_threshold=(starting_num_outputs_on - 1))
        keyvals['outputs_on_after_screen_off'] = num_outputs_on
        if num_outputs_on >= starting_num_outputs_on:
            num_errors += 1
            logging.error('At least one display must have been turned off. ' + \
                          'Number of displays on: %s' % num_outputs_on)
        else:
            self._wait_for_stable_energy_rate()
            keyvals['screen_off_power_w'] = self._get_current_energy_rate()

        # Set brightness to max.
        self._backlight_controller.set_brightness_to_max()
        current_brightness = utils.wait_for_value(
            self._backlight.get_level, max_threshold=keyvals['max_brightness'])
        if current_brightness != keyvals['max_brightness']:
            num_errors += 1
            logging.error(('Failed to increase brightness to max, ' + \
                           'brightness is %d.') % current_brightness)

        # Verify that the same number of outputs are on as before.
        num_outputs_on = get_num_outputs_on()
        keyvals['outputs_on_at_end'] = num_outputs_on
        if num_outputs_on != starting_num_outputs_on:
            num_errors += 1
            logging.error(('Number of displays turned on should be same as ' + \
                           'at start.  Number of displays on: %s') %
                          num_outputs_on)

        self._wait_for_stable_energy_rate()
        keyvals['final_power_w'] = self._get_current_energy_rate()

        # Energy rate must have changed significantly between transitions.
        if 'max_brightness_power_w' in keyvals and \
           'min_brightness_power_w' in keyvals and \
           keyvals['min_brightness_power_w'] >= \
               keyvals['max_brightness_power_w'] * max_to_min_factor:
            num_errors += 1
            logging.error('Power draw did not decrease enough when ' + \
                          'brightness was decreased from max to min.')

        if 'screen_off_power_w' in keyvals and \
           'min_brightness_power_w' in keyvals and \
           keyvals['screen_off_power_w'] >= \
               keyvals['min_brightness_power_w'] * min_to_off_factor:
            num_errors += 1
            logging.error('Power draw did not decrease enough when screen ' + \
                          'was turned off.')

        if num_outputs_on == starting_num_outputs_on and \
           'screen_off_power_w' in keyvals and \
           keyvals['final_power_w'] <= \
               keyvals['screen_off_power_w'] * off_to_max_factor:
            num_errors += 1
            logging.error('Power draw did not increase enough after ' + \
                          'turning screen on.')

        self.write_perf_keyval(keyvals)

        if num_errors > 0:
            raise error.TestFail('Test failed with %d errors' % num_errors)


    def cleanup(self):
        if self._backlight:
            self._backlight.restore()
        super(power_BacklightControl, self).cleanup()


    def _get_current_energy_rate(self):
        return power_status.get_status().battery[0].energy_rate


    def _wait_for_stable_energy_rate(self,
                                     max_variation_percent=5,
                                     sample_delay_sec=1,
                                     window_size=10,
                                     timeout_sec=30):
        """
        Waits for the energy rate to stablize.  Stability criterion:
            The last |window_size| samples of energy rate do not deviate from
            their mean by more than |max_variation_percent|.

        Arguments:
            max_variation_percent   Percentage of allowed deviation from mean
                                    energy rate to still be considered stable.
            sample_delay_sec        Time to wait between each reading of the
                                    energy rate.
            window_size             Number of energy rate samples required to
                                    measure stability.  If there are more
                                    samples than this amount, use only the last
                                    |window_size| values.
            timeout_sec             If stability has not been attained after
                                    this long, stop waiting.

        Return value:
            True if energy rate stabilized before timeout.
            False if timed out waiting for energy rate to stabilize.
        """
        start_time = time.time()
        samples = []
        max_variation_factor = max_variation_percent / 100.0
        while time.time() - start_time < timeout_sec:
            current_rate = self._get_current_energy_rate()

            # Remove the oldest value if the list of energy rate samples is at
            # the maximum limit |window_size|, before appending a new value.
            if len(samples) >= window_size:
                samples = samples[1:]
            samples.append(current_rate)

            mean = sum(samples) / len(samples)
            if len(samples) >= window_size and \
               max(samples) <= mean * (1 + max_variation_factor) and \
               min(samples) >= mean * (1 - max_variation_factor):
                return True

            time.sleep(sample_delay_sec)

        return False
