# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import math
import time

from autotest_lib.client.common_lib import error
from autotest_lib.server.cros.faft.firmware_test import FirmwareTest
from autotest_lib.server.cros.servo import pd_console


class firmware_PDVbusRequest(FirmwareTest):
    """
    Servo based USB PD VBUS level test. This test is written to use both
    the DUT and Plankton test board. It requires that the DUT support
    dualrole (SRC or SNK) operation. VBUS change requests occur in two
    methods. First, with the DUT in SNK mode, it uses the pd console command
    'pd 0/1 dev V' command where V is the desired voltage 5/12/20. The 2nd
    test initiates the VBUS change by using special Plankton feature to
    send new SRC CAP message. This causes the DUT to request a new VBUS
    voltage mathcing what's in the SRC CAP message.

    Pass critera is all voltage transitions are successful.

    """
    version = 1

    PD_SETTLE_DELAY = 4
    USBC_SINK_VOLTAGE = 5
    USBC_MAX_VOLTAGE = 20
    VBUS_TOLERANCE = 0.12

    VOLTAGE_SEQUENCE = [5, 12, 20, 12, 5, 20, 5, 5, 12, 12, 20]

    def _compare_vbus(self, expected_vbus_voltage):
        """Check VBUS using plankton

        @param expected_vbus_voltage: nominal VBUS level (in volts)

        @returns: a tuple containing pass/fail indication and logging string
        """
        # Get Vbus voltage and current
        vbus_voltage = self.plankton.vbus_voltage
        vbus_current = self.plankton.vbus_current
        # Compute voltage tolerance range
        tolerance = self.VBUS_TOLERANCE * expected_vbus_voltage
        voltage_difference = math.fabs(expected_vbus_voltage - vbus_voltage)
        result_str = 'Target = %02dV:\tAct = %.2f\tDelta = %.2f' % \
                     (expected_vbus_voltage, vbus_voltage, voltage_difference)
        # Verify that measured Vbus voltage is within expected range
        voltage_difference = math.fabs(expected_vbus_voltage - vbus_voltage)
        if voltage_difference > tolerance:
            result = 'FAIL'
        else:
            result = 'PASS'
        return result, result_str

    def initialize(self, host, cmdline_args):
        super(firmware_PDVbusRequest, self).initialize(host, cmdline_args)
        # Only run in normal mode
        self.switcher.setup_mode('normal')
        self.usbpd.send_command('chan 0')

    def cleanup(self):
        self.usbpd.send_command('chan 0xffffffff')
        super(firmware_PDVbusRequest, self).cleanup()

    def run_once(self):
        """Exectue VBUS request test.

        """

        # create objects for pd utilities
        pd_dut_utils = pd_console.PDConsoleUtils(self.usbpd)
        pd_plankton_utils = pd_console.PDConsoleUtils(self.plankton)

        # Make sure PD support exists in the UART console
        if pd_dut_utils.verify_pd_console() == False:
            raise error.TestFail("pd command not present on console!")

        # Type C connection (PD contract) should exist at this point
        dut_state = pd_dut_utils.query_pd_connection()
        logging.info('DUT PD connection state: %r', dut_state)
        if dut_state['connect'] == False:
            raise error.TestFail("pd connection not found")
        if dut_state['role'] != pd_dut_utils.SNK_CONNECT:
            # DUT needs to be in SINK Mode, attempt to force change
            pd_dut_utils.set_pd_dualrole('snk')
            time.sleep(self.PD_SETTLE_DELAY)
            if pd_dut_utils.get_pd_state(dut_state['port']) != pd_dut_utils.SNK_CONNECT:
                raise error.TestFail("DUT not able to connect in SINK mode")

        # Plankton must be set to 20V SRC mode in order for the DUT
        # to be able to request all 3 possible voltage levels (5, 12, 20).
        # The DUT must be in SNK mode for the pd <port> dev <voltage>
        # command to have an effect.
        self.plankton.charge(self.USBC_MAX_VOLTAGE)
        time.sleep(self.PD_SETTLE_DELAY)
        logging.info('Start of DUT initiated tests')
        dut_failures = []
        for v in self.VOLTAGE_SEQUENCE:
            # Build 'pd <port> dev <voltage> command
            cmd = 'pd %d dev %d' % (dut_state['port'], v)
            pd_dut_utils.send_pd_command(cmd)
            time.sleep(self.PD_SETTLE_DELAY)
            result, result_str = self._compare_vbus(v)
            logging.info('%s, %s', result_str, result)
            if result == 'FAIL':
                dut_failures.append(result_str)

        # Make sure Plankton is set back to 20VSRC so DUT will accept all options
        cmd = 'pd %d dev %d' % (dut_state['port'], self.USBC_MAX_VOLTAGE)
        time.sleep(self.PD_SETTLE_DELAY)
        # The next group of tests need DUT to connect in SNK and SRC modes
        pd_dut_utils.set_pd_dualrole('on')

        plankton_failures = []
        logging.info('Start Plankton initiated tests')
        for voltage in self.plankton.get_charging_voltages():
            logging.info('********* %r *********', voltage)
            # Set charging voltage
            self.plankton.charge(voltage)
            # Wait for new PD contract to be established
            time.sleep(self.PD_SETTLE_DELAY)
            # Get current Plankton PD state
            plankton_state = pd_plankton_utils.get_pd_state(0)
            expected_vbus_voltage = self.plankton.charging_voltage
            # If Plankton is sink, then Vbus_exp = 5v
            if plankton_state == pd_plankton_utils.SNK_CONNECT:
                expected_vbus_voltage = self.USBC_SINK_VOLTAGE
            result, result_str = self._compare_vbus(expected_vbus_voltage)
            logging.info('%s, %s', result_str, result)
            if result == 'FAIL':
                plankton_failures.append(result_str)

        if dut_failures:
            logging.error('DUT voltage request failures')
            for fail in dut_failures:
                logging.error('%s', fail)

        if plankton_failures:
            logging.error('Plankton voltage source cap failures')
            for fail in plankton_failures:
                logging.error('%s', fail)

        if dut_failures or plankton_failures:
            if dut_failures and plankton_failures:
                test = 'DUT and Plankton'
                number = len(dut_failures) + len(plankton_failures)
            elif dut_failures:
                test = 'DUT'
                number = len(dut_failures)
            else:
                test = 'Plankton'
                number = len(plankton_failures)
            raise error.TestFail('%s failed %d times' % (test, number))
