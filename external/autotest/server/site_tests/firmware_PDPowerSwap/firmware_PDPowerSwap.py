# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import time

from autotest_lib.client.common_lib import error
from autotest_lib.server.cros.faft.firmware_test import FirmwareTest
from autotest_lib.server.cros.servo import pd_console


class firmware_PDPowerSwap(FirmwareTest):
    """
    Servo based USB PD power role swap test.

    Pass critera is all power role swaps are successful if the DUT
    is dualrole capable. If not dualrole, then pass criteria is
    the DUT sending a reject message in response to swap request.

    """
    version = 1

    PD_ROLE_DELAY = 0.5
    PD_CONNECT_DELAY = 4
    PLANKTON_PORT = 0
    POWER_SWAP_ITERATIONS = 5
    # Source power role
    SRC ='SRC_READY'
    # Sink power role
    SNK = 'SNK_READY'

    def _set_plankton_power_role_to_src(self):
        """Force Plankton to act as a source

        @returns True if Plankton power role is source, false otherwise
        """
        PLANKTON_SRC_VOLTAGE = 20
        self.plankton.charge(PLANKTON_SRC_VOLTAGE)
        # Wait for change to take place
        time.sleep(self.PD_CONNECT_DELAY)
        plankton_state = self.plankton_pd_utils.get_pd_state(self.PLANKTON_PORT)
        # Current Plankton power role should be source
        return bool(plankton_state == self.SRC)

    def _send_power_swap_get_reply(self, console, port):
        """Send power swap request, get PD control msg reply

        The PD console debug mode is enabled prior to sending
        a pd power role swap request message. This allows the
        control message reply to be extracted. The debug mode
        is disabled prior to exiting.

        @param console: pd console object for uart access

        @returns: PD control header message
        """
        # Enable PD console debug mode to show control messages
        console.enable_pd_console_debug()
        cmd = 'pd %d swap power' % port
        m = console.send_pd_command_get_output(cmd, ['RECV\s([\w]+)'])
        ctrl_msg = int(m[0][1], 16) & console.PD_CONTROL_MSG_MASK
        console.disable_pd_console_debug()
        return ctrl_msg

    def _attempt_power_swap(self, pd_port, direction):
        """Perform a power role swap request

        Initiate a power role swap request on either the DUT or
        Plankton depending on the direction parameter. The power
        role swap is then verified to have taken place.

        @param pd_port: DUT pd port value 0/1
        @param direction: rx or tx from the DUT perspective

        @returns True if power swap is successful
        """
        # Get DUT current power role
        dut_pr = self.dut_pd_utils.get_pd_state(pd_port)
        if direction == 'rx':
            console = self.plankton_pd_utils
            port = self.PLANKTON_PORT
        else:
            console = self.dut_pd_utils
            port = pd_port
        # Send power swap request
        self._send_power_swap_get_reply(console, port)
        time.sleep(self.PD_CONNECT_DELAY)
        # Get Plankton power role
        plankton_pr = self.plankton_pd_utils.get_pd_state(self.PLANKTON_PORT)
        return bool(dut_pr == plankton_pr)

    def _test_power_swap_reject(self, pd_port):
        """Verify that a power swap request is rejected

        This tests the case where the DUT isn't in dualrole mode.
        A power swap request is sent by Plankton, and then
        the control message checked to ensure the request was rejected.
        In addition, the connection state is verified to not have
        changed.

        @param pd_port: port for DUT pd connection
        """
        # Get current DUT power role
        dut_power_role = self.dut_pd_utils.get_pd_state(pd_port)
        # Send swap command from Plankton and get reply
        ctrl_msg = self._send_power_swap_get_reply(self.plankton_pd_utils,
                                                   self.PLANKTON_PORT)
        if ctrl_msg != self.dut_pd_utils.PD_CONTROL_MSG_DICT['Reject']:
            raise error.TestFail('Power Swap Req not rejected, returned %r' %
                                 ctrl_msg)
        # Get DUT current state
        pd_state = self.dut_pd_utils.get_pd_state(pd_port)
        if pd_state != dut_power_role:
            raise error.TestFail('PD not connected! pd_state = %r' %
                                 pd_state)

    def initialize(self, host, cmdline_args):
        super(firmware_PDPowerSwap, self).initialize(host, cmdline_args)
        # Only run in normal mode
        self.switcher.setup_mode('normal')
        # Turn off console prints, except for USBPD.
        self.usbpd.send_command('chan 0x08000000')

    def cleanup(self):
        self.usbpd.send_command('chan 0xffffffff')
        super(firmware_PDPowerSwap, self).cleanup()

    def run_once(self):
        """Execute Power Role swap test.

        1. Verify that pd console is accessible
        2. Verify that DUT has a valid PD contract and connected to Plankton
        3. Determine if DUT is in dualrole mode
        4. If not dualrole mode, verify DUT rejects power swap request
           Else test power swap (tx/rx), then Force DUT to be sink or
           source only and verify rejecttion of power swap request.

        """
        # create objects for pd utilities
        self.dut_pd_utils = pd_console.PDConsoleUtils(self.usbpd)
        self.plankton_pd_utils = pd_console.PDConsoleUtils(self.plankton)
        self.connect_utils = pd_console.PDConnectionUtils(self.dut_pd_utils,
                                                          self.plankton_pd_utils)

        # Make sure PD support exists in the UART console
        if self.dut_pd_utils.verify_pd_console() == False:
            raise error.TestFail("pd command not present on console!")

        # Type C connection (PD contract) should exist at this point
        # For this test, the DUT must be connected to a Plankton.
        pd_port = self.connect_utils.find_dut_to_plankton_connection()
        if pd_port is None:
            raise error.TestFail("DUT to Plankton PD connection not found")
        dut_connect_state = self.dut_pd_utils.get_pd_state(pd_port)
        logging.info('Initial DUT connect state = %s', dut_connect_state)

        # Get DUT dualrole status
        if self.dut_pd_utils.is_pd_dual_role_enabled() == False:
            # DUT does not support dualrole mode, power swap
            # requests to the DUT should be rejected.
            logging.info('Power Swap support not advertised by DUT')
            self._test_power_swap_reject(pd_port)
            logging.info('Power Swap request rejected by DUT as expected')
        else:
            # Start with Plankton as source
            if self._set_plankton_power_role_to_src() == False:
                raise error.TestFail('Plankton not set to source')
            # DUT is dualrole in dual role mode. Test power role swap
            # operation intiated both by the DUT and Plankton.
            success = 0
            for attempt in xrange(self.POWER_SWAP_ITERATIONS):
                if attempt & 1:
                    direction = 'rx'
                else:
                    direction = 'tx'
                if self._attempt_power_swap(pd_port, direction):
                    success += 1
                new_state = self.dut_pd_utils.get_pd_state(pd_port)
                logging.info('New DUT power role = %s', new_state)

            if success != self.POWER_SWAP_ITERATIONS:
                raise error.TestFail('Failed %r power swap attempts' %
                                     (self.POWER_SWAP_ITERATIONS - success))

            # Force DUT to only support current power role
            if new_state == self.SRC:
                dual_mode = 'src'
            else:
                dual_mode = 'snk'
            logging.info('Setting dualrole mode to %s', dual_mode)
            self.dut_pd_utils.set_pd_dualrole(dual_mode)
            time.sleep(self.PD_ROLE_DELAY)
            # Expect behavior now is that DUT will reject power swap
            self._test_power_swap_reject(pd_port)
            logging.info('Power Swap request rejected by DUT as expected')
            # Restore DUT dual role operation
            self.dut_pd_utils.set_pd_dualrole('on')
            # Set connection back to default arrangement
            self.plankton_pd_utils.set_pd_dualrole('off')
            self.plankton_pd_utils.send_pd_command('fake disconnect 100 1000')

