# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, threading, time

from autotest_lib.client.bin import utils
from autotest_lib.client.cros import service_stopper


# List of thermal throttling services that should be disabled.
# - temp_metrics for link.
# - thermal for daisy, snow, pit etc.
# TODO(ihf): cpu_quiet on nyan isn't a service. We still need to disable it
#            on nyan. See crbug.com/357457.
_THERMAL_SERVICES = ['temp_metrics', 'thermal']


class PerfControl(object):
    """
    Provides methods for setting the performance mode of a device.

    In particular it verifies the machine is idle and cold and tries to set
    it into a consistent, high performance state during initialization.

    Furthermore it monitors the state of the machine (in particular
    temperature) and verifies that nothing bad happened along the way.

    Example usage:

    with PerfControl() as pc:
        if not pc.verify_is_valid():
            raise error.TestError(pc.get_error_reason())
        # Do all performance testing.
        ...
        if not pc.verify_is_valid():
            raise error.TestError(pc.get_error_reason())
    """
    def __init__(self):
        self._service_stopper = None
        # Keep a copy of the current state for cleanup.
        self._temperature_init = utils.get_current_temperature_max()
        self._temperature_critical = utils.get_temperature_critical()
        self._original_governors = utils.set_high_performance_mode()
        self._error_reason = None
        if not utils.wait_for_idle_cpu(60.0, 0.1):
            self._error_reason = 'Could not get idle CPU.'
            return
        if not utils.wait_for_cool_machine():
            self._error_reason = 'Could not get cold machine.'
            return
        self._temperature_cold = utils.get_current_temperature_max()
        self._temperature_max = self._temperature_cold
        threading.Thread(target=self._monitor_performance_state).start()
        # Should be last just in case we had a runaway process.
        self._stop_thermal_throttling()


    def __enter__(self):
        return self


    def __exit__(self, _type, value, traceback):
        # First thing restart thermal management.
        self._restore_thermal_throttling()
        utils.restore_scaling_governor_states(self._original_governors)


    def get_error_reason(self):
        """
        Returns an error reason string if we encountered problems to pass
        on to harness/wmatrix.
        """
        return self._error_reason


    def verify_is_valid(self):
        """
        For now we declare performance results as valid if
        - we did not have an error before.
        - the monitoring thread never saw temperatures too close to critical.

        TODO(ihf): Search log files for thermal throttling messages like in
                   src/build/android/pylib/perf/thermal_throttle.py
        """
        if self._error_reason:
            return False
        temperature_bad = self._temperature_critical - 1.0
        logging.info("Max observed temperature = %.1f'C (bad limit = %.1f'C)",
                     self._temperature_max, temperature_bad)
        if (self._temperature_max > temperature_bad):
            self._error_reason = 'Machine got hot during testing.'
            return False
        return True


    def _monitor_performance_state(self):
        """
        Checks machine temperature once per second.
        TODO(ihf): make this more intelligent with regards to governor,
                   CPU, GPU and maybe zram as needed.
        """
        while True:
            time.sleep(1)
            current_temperature = utils.get_current_temperature_max()
            self._temperature_max = max(self._temperature_max,
                                        current_temperature)
            # TODO(ihf): Remove this spew once PerfControl is stable.
            logging.info('PerfControl CPU temperature = %.1f',
                          current_temperature)


    def _stop_thermal_throttling(self):
        """
        If exist on the platform/machine it stops the different thermal
        throttling scripts from running.
        Warning: this risks abnormal behavior if machine runs in high load.
        """
        self._service_stopper = service_stopper.ServiceStopper(
                                                    _THERMAL_SERVICES)


    def _restore_thermal_throttling(self):
        """
        Restores the original thermal throttling state.
        """
        if self._service_stopper:
            self._service_stopper.restore_services()
