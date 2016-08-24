# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import re
import time

from autotest_lib.client.common_lib import error
from autotest_lib.server.cros.faft.firmware_test import FirmwareTest
from autotest_lib.server.cros.servo import pd_console


class firmware_PDDataSwap(FirmwareTest):
    """
    Servo based USB PD data role swap test

    Pass critera is all data role swaps complete, or
    a reject control message is received from the DUT in the
    cases where the swap does not complete.

    """
    version = 1

    PD_ROLE_DELAY = 0.5
    PD_CONNECT_DELAY = 4
    PLANKTON_PORT = 0
    DATA_SWAP_ITERATIONS = 10
    # Upward facing port data role
    UFP = 'UFP'
    # Downward facing port data role
    DFP = 'DFP'
    # Plankton initiated data swap request
    PLANKTON_SWAP_REQ = 'pd %d swap data' % PLANKTON_PORT
    # Swap Result Tables
    swap_attempt = {
        ('rx', DFP): 0,
        ('rx', UFP): 0,
        ('tx', DFP): 0,
        ('tx', UFP): 0
    }
    swap_failure = {
        ('rx', DFP): 0,
        ('rx', UFP): 0,
        ('tx', DFP): 0,
        ('tx', UFP): 0
    }

    def _verify_plankton_connection(self, port):
        """Verify if DUT to Plankton PD connection

        This method checks for a Plankton PD connection for the
        given port by first verifying if a PD connection is present.
        If found, then it uses a Plankton feature to force a PD disconnect.
        If the port is no longer in the connected state, and following
        a delay, is found to be back in the connected state, then
        a DUT pd to Plankton connection is verified.

        @param port: DUT pd port to test

        @returns True if DUT to Plankton pd connection is verified
        """
        DISCONNECT_TIME_SEC = 2
        # plankton console command to force PD disconnect
        disc_cmd = 'fake_disconnect 100 %d' % (DISCONNECT_TIME_SEC*1000)
        # Only check for Plankton if DUT has active PD connection
        if self.dut_pd_utils.is_pd_connected(port):
            # Attempt to force PD disconnection
            self.plankton_pd_utils.send_pd_command(disc_cmd)
            time.sleep(self.PD_ROLE_DELAY)
            # Verify that DUT PD port is no longer connected
            if self.dut_pd_utils.is_pd_connected(port) == False:
                # Wait for disconnect timer and give time to reconnect
                time.sleep(self.PD_CONNECT_DELAY + DISCONNECT_TIME_SEC)
                if self.dut_pd_utils.is_pd_connected(port):
                    logging.info('Plankton connection verfied on port %d', port)
                    return True
            else:
                # Could have disconnected other port, allow it to reconnect
                # before exiting.
                time.sleep(self.PD_CONNECT_DELAY + DISCONNECT_TIME_SEC)
        return False

    def _find_dut_to_plankton_connection(self):
        """Find the PD port which is connected to Plankton

        @returns DUT pd port number if found, None otherwise
        """
        for port in xrange(self.dut_pd_utils.PD_MAX_PORTS):
            # Check for DUT to Plankton connection on port
            if self._verify_plankton_connection(port):
                # Plankton PD connection found so exit
                return port
        return None

    def _get_data_role(self, console, port):
        """Get data role of PD connection

        @param console: pd console object for uart access
        @param port: 0/1 pd port of current connection

        @returns: 'DFP' or 'UFP'
        """
        role = console.get_pd_role(port)
        m = re.search('[\w]+-([\w]+)', role)
        return m.group(1)

    def _get_remote_role(self, local_role):
        """Invert data role

        @param local_role: data role to be flipped

        @returns: flipped data role value
        """
        if local_role == self.DFP:
            return self.UFP
        else:
            return self.DFP

    def _change_dut_power_role(self, port):
        """Force power role change via Plankton

        @param port: port of DUT PD connection

        @returns True is power role change is successful
        """
        PLANKTON_SRC_VOLTAGE = 5
        PLANKTON_SNK_VOLTAGE = 0
        pd_state = self.dut_pd_utils.get_pd_state(port)
        if pd_state == self.dut_pd_utils.SRC_CONNECT:
            # DUT is currently a SRC, so change to SNK
            # Use Plankton method to ensure power role change
            self.plankton.charge(PLANKTON_SRC_VOLTAGE)
        else:
            # DUT is currently a SNK, so change it to a SRC.
            self.plankton.charge(PLANKTON_SNK_VOLTAGE)
        # Wait for change to take place
        time.sleep(self.PD_CONNECT_DELAY)
        plankton_state = self.plankton_pd_utils.get_pd_state(0)
        # Current Plankton state should equal DUT state when called
        return bool(pd_state == plankton_state)

    def _send_data_swap_get_reply(self, console, port):
        """Send data swap request, get PD control msg reply

        The PD console debug mode is enabled prior to sending
        a pd data role swap request message. This allows the
        control message reply to be extracted. The debug mode
        is disabled prior to exiting.

        @param console: pd console object for uart access

        @ returns: PD control header message
        """
        # Enable PD console debug mode to show control messages
        console.enable_pd_console_debug()
        cmd = 'pd %d swap data' % port
        m = console.send_pd_command_get_output(cmd, ['RECV\s([\w]+)'])
        ctrl_msg = int(m[0][1], 16) & console.PD_CONTROL_MSG_MASK
        console.disable_pd_console_debug()
        return ctrl_msg

    def _attempt_data_swap(self, pd_port, direction):
        """Perform a data role swap request

        Data swap requests can be either initiated by the DUT or received
        by the DUT. This direction determines which PD console is used
        to initiate the swap command. The data role before and after
        the swap command are compared to determine if it took place.

        Even if data swap capability is advertised, a PD device is allowed
        to reject the request. Therefore, not swapping isn't itself a
        failure. When Plankton is used to initate the request, the debug
        mode is enabled which allows the control message from the DUT to
        be analyzed. If the swap does not occur, but the request is rejected
        by the DUT then that is not counted as a failure.

        @param pd_port: DUT pd port value 0/1
        @param direction: rx or tx from the DUT perspective

        @returns PD control reply message for tx swaps, 0 otherwise
        """
        # Get starting DUT data role
        dut_dr = self._get_data_role(self.dut_pd_utils, pd_port)
        self.swap_attempt[(direction, dut_dr)] += 1
        if direction == 'tx':
            # Initiate swap request from the DUT
            console = self.dut_pd_utils
            cmd = 'pd %d swap data' % pd_port
            # Send the 'swap data' command
            self.dut_pd_utils.send_pd_command(cmd)
            # Not using debug mode, so there is no reply message
            ctrl = 0
        else:
            # Initiate swap request from Plankton
            console = self.plankton_pd_utils
            ctrl  = self._send_data_swap_get_reply(console, self.PLANKTON_PORT)

        time.sleep(self.PD_ROLE_DELAY)
        # Get DUT current data role
        swap_dr = self._get_data_role(self.dut_pd_utils, pd_port)
        logging.info('%s swap attempt: prev = %s, new = %s, msg = %s',
                      direction, dut_dr, swap_dr, ctrl)
        if (dut_dr == swap_dr and
                ctrl != self.dut_pd_utils.PD_CONTROL_MSG_DICT['Reject']):
            self.swap_failure[(direction, dut_dr)] += 1
        return ctrl

    def _execute_data_role_swap_test(self, pd_port):
        """Execute a series of data role swaps

        Attempt both rx and tx data swaps, from perspective of DUT.
        Even if the DUT advertises support, it can
        reject swap requests when already in the desired data role. For
        example many devices will not swap if already in DFP mode.
        However, Plankton should always accept a request. Therefore,
        when a swap failed on a rx swap, then that is followed by
        a tx swap attempt.

        @param pd_port: port number of DUT PD connection
        """
        for attempt in xrange(self.DATA_SWAP_ITERATIONS):
            # Use the same direction for every 2 loop iterations
            if attempt & 2:
                direction = 'tx'
            else:
                direction = 'rx'
            ctrl_msg = self._attempt_data_swap(pd_port, direction)
            if (direction == 'rx' and
                    ctrl_msg ==
                    self.dut_pd_utils.PD_CONTROL_MSG_DICT['Reject']):
                # Use plankton initated swap to change roles
                self._attempt_data_swap(pd_port, 'tx')

    def _test_data_swap_reject(self, pd_port):
        """Verify that data swap request is rejected

        This tests the case where the DUT doesn't advertise support
        for data swaps. A data request is sent by Plankton, and then
        the control message checked to ensure the request was rejected.
        In addition, the data role and connection state are verified
        to remain unchanged.

        @param pd_port: port for DUT pd connection
        """
        # Get current DUT data role
        dut_data_role = self._get_data_role(self.dut_pd_utils, pd_port)
        dut_connect_state = self.dut_pd_utils.get_pd_state(pd_port)
        # Send swap command from Plankton and get reply
        ctrl_msg = self._send_data_swap_get_reply(self.plankton_pd_utils,
                                                  self.PLANKTON_PORT)
        if ctrl_msg != self.dut_pd_utils.PD_CONTROL_MSG_DICT['Reject']:
            raise error.TestFail('Data Swap Req not rejected, returned %r' %
                                 ctrl_msg)
        # Get DUT current state
        pd_state = self.dut_pd_utils.get_pd_state(pd_port)
        if pd_state != dut_connect_state:
            raise error.TestFail('PD not connected! pd_state = %r' %
                                 pd_state)
        # Since reject message was received, verify data role didn't change
        curr_dr = self._get_data_role(self.dut_pd_utils, pd_port)
        if curr_dr != dut_data_role:
            raise error.TestFail('Unexpected PD data role change')

    def initialize(self, host, cmdline_args):
        super(firmware_PDDataSwap, self).initialize(host, cmdline_args)
        # Only run in normal mode
        self.switcher.setup_mode('normal')
        self.usbpd.send_command('chan 0')

    def cleanup(self):
        self.usbpd.send_command('chan 0xffffffff')
        super(firmware_PDDataSwap, self).cleanup()

    def run_once(self):
        """Exectue Data Role swap test.

        1. Verify that pd console is accessible
        2. Verify that DUT has a valid PD contract
        3. Determine if DUT advertises support for data swaps
        4. Test DUT initiated and received data swaps
        5. Swap power roles if supported
        6. Repeat DUT received data swap requests

        """
        # create objects for pd utilities
        self.dut_pd_utils = pd_console.PDConsoleUtils(self.usbpd)
        self.plankton_pd_utils = pd_console.PDConsoleUtils(self.plankton)

        # Make sure PD support exists in the UART console
        if self.dut_pd_utils.verify_pd_console() == False:
            raise error.TestFail("pd command not present on console!")

        # Type C connection (PD contract) should exist at this point
        # For this test, the DUT must be connected to a Plankton.
        pd_port = self._find_dut_to_plankton_connection()
        if pd_port == None:
            raise error.TestFail("DUT to Plankton PD connection not found")
        dut_connect_state = self.dut_pd_utils.get_pd_state(pd_port)
        logging.info('Initial DUT connect state = %s', dut_connect_state)

        # Determine if DUT supports data role swaps
        dr_swap_allowed = self.plankton_pd_utils.is_pd_flag_set(
                self.PLANKTON_PORT, 'data_swap')
        # Get current DUT data role
        dut_data_role = self._get_data_role(self.dut_pd_utils, pd_port)
        logging.info('Starting DUT Data Role = %r', dut_data_role)

        # If data swaps are not allowed on the DUT, then still
        # attempt a data swap and verify that the request is
        # rejected by the DUT and that it remains connected and
        # in the same role.
        if dr_swap_allowed == False:
            logging.info('Data Swap support not advertised by DUT')
            self._test_data_swap_reject(pd_port)
            logging.info('Data Swap request rejected by DUT as expected')
        else:
            # Data role swap support advertised, test this feature.
            self._execute_data_role_swap_test(pd_port)

            # If DUT supports Power Role swap then attempt to change roles.
            # This way, data role swaps will be tested in both configurations.
            if self.plankton_pd_utils.is_pd_flag_set(
                     self.PLANKTON_PORT, 'power_swap'):
                logging.info('\nDUT advertises Power Swap Support')
                # Attempt to swap power roles
                power_swap = self._change_dut_power_role(pd_port)
                if power_swap:
                    self._execute_data_role_swap_test(pd_port)
                else:
                    logging.warn('Power swap not successful!')
                    logging.warn('Only tested with DUT in %s state',
                                 dut_connect_state)
            else:
                logging.info('DUT does not advertise power swap support')

            logging.info('***************** Swap Results ********************')
            total_attempts = 0
            total_failures = 0
            for direction, role in self.swap_attempt.iterkeys():
                logging.info('%s %s swap attempts = %d, failures = %d',
                             direction, role,
                             self.swap_attempt[(direction, role)],
                             self.swap_failure[(direction, role)])
                total_attempts += self.swap_attempt[(direction, role)]
                total_failures += self.swap_failure[(direction, role)]

            # If any swap attempts were not successful, flag test as failure
            if total_failures:
                raise error.TestFail('Data Swap Fail: Attempt = %d, Failure = %d' %
                                 (total_attempts, total_failures))
