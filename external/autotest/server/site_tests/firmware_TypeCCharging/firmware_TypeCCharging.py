# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This is a USB type C charging test using Plankton board."""

import logging
import math
import time

from autotest_lib.client.common_lib import error
from autotest_lib.server.cros.faft.firmware_test import FirmwareTest
from autotest_lib.server.cros.servo import pd_console

class firmware_TypeCCharging(FirmwareTest):
    """USB type C charging test."""
    version = 1

    USBC_SINK_VOLTAGE = 5
    VBUS_TOLERANCE = 0.12
    VBUS_5V_CURRENT_RANGE = (2, 3.4)
    VBUS_CHANGE_DELAY = 4


    def run_once(self):
        """Compares VBUS voltage and current with charging setting.

        When charging voltage == 0, Plankton will act as a power sink and draws
        5 volts from DUT. Other charging voltage should be seen on USB type C
        VBUS INA meter in a 12% range.

        When charging voltage == 5, Plankton INA current should be seen around
        3 Amps (we set the range among 2 ~ 3.4 Amps just as in factory testing).
        Other positive charging votage should not be less than 0 Amp.

        @raise TestFail: If VBUS voltage or current is not in range.
        """
        self.pd_console_utils = pd_console.PDConsoleUtils(self.plankton)

        for charging_voltage in self.plankton.get_charging_voltages():
            self.plankton.charge(charging_voltage)
            time.sleep(self.VBUS_CHANGE_DELAY)
            pd_state = self.pd_console_utils.get_pd_state(0)
            if charging_voltage > 0:
                 expected_state = self.pd_console_utils.SRC_CONNECT
            else:
                 expected_state = self.pd_console_utils.SNK_CONNECT
            logging.info('Plankton state = %s', pd_state)
            if pd_state != expected_state:
                raise error.TestFail('PD state != expected state, (%s != %s)' %
                                     (pd_state, expected_state))
            expected_vbus_voltage = float(
                    charging_voltage if charging_voltage > 0 else
                    self.USBC_SINK_VOLTAGE)
            tolerance = self.VBUS_TOLERANCE * expected_vbus_voltage
            vbus_voltage = self.plankton.vbus_voltage
            vbus_current = self.plankton.vbus_current
            logging.info('Charging %dV: VBUS V=%f I=%f', charging_voltage,
                         vbus_voltage, vbus_current)

            if math.fabs(expected_vbus_voltage - vbus_voltage) > tolerance:
                raise error.TestFail(
                        'VBUS voltage out of range: %f (%f, delta %f)' %
                        (vbus_voltage, expected_vbus_voltage, tolerance))

            if charging_voltage == 0 and vbus_current > 0:
                raise error.TestFail('Failed to consume power from DUT')

            if charging_voltage > 0 and vbus_current <= 0:
                raise error.Testfail(
                        'VBUS current less than 0 in %d volt: %f' %
                        (charging_voltage, vbus_current))

            if (charging_voltage == 5 and
                (vbus_current < self.VBUS_5V_CURRENT_RANGE[0] or
                 vbus_current > self.VBUS_5V_CURRENT_RANGE[1])):
                raise error.TestFail(
                        'VBUS current out of range in 5 volt: %f %r' %
                        (vbus_current, self.VBUS_5V_CURRENT_RANGE))
