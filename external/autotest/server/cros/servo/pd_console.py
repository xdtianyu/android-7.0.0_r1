# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import re
import logging
import time

from autotest_lib.client.common_lib import error

class PDConsoleUtils(object):
    """ Provides a set of methods common to USB PD FAFT tests

    Each instance of this class is associated with a particular
    servo UART console. USB PD tests will typically use the console
    command 'pd' and its subcommands to control/monitor Type C PD
    connections. The servo object used for UART operations is
    passed in and stored when this object is created.

    """

    SRC_CONNECT = 'SRC_READY'
    SNK_CONNECT = 'SNK_READY'
    SRC_DISC = 'SRC_DISCONNECTED'
    SNK_DISC = 'SNK_DISCONNECTED'
    PD_MAX_PORTS = 2
    CONNECT_TIME = 4

    # dualrole input/ouput values
    DUALROLE_QUERY_DELAY = 0.25
    dual_index = {'on': 0, 'off': 1, 'snk': 2, 'src': 3}
    dualrole_cmd = ['on', 'off', 'sink', 'source']
    dualrole_resp = ['on', 'off', 'force sink', 'force source']

    # Dictionary for 'pd 0/1 state' parsing
    PD_STATE_DICT = {
        'port': 'Port\s+([\w]+)',
        'role': 'Role:\s+([\w]+-[\w]+)',
        'pd_state': 'State:\s+([\w]+_[\w]+)',
        'flags': 'Flags:\s+([\w]+)',
        'polarity': '(CC\d)'
    }

    # Dictionary for PD control message types
    PD_CONTROL_MSG_MASK = 0x1f
    PD_CONTROL_MSG_DICT = {
        'GoodCRC': 1,
        'GotoMin': 2,
        'Accept': 3,
        'Reject': 4,
        'Ping': 5,
        'PS_RDY': 6,
        'Get_Source_Cap': 7,
        'Get_Sink_Cap': 8,
        'DR_Swap': 9,
        'PR_Swap': 10,
        'VCONN_Swap': 11,
        'Wait': 12,
        'Soft_Reset': 13
    }

    # Dictionary for PD firmware state flags
    PD_STATE_FLAGS_DICT = {
        'power_swap': 1 << 1,
        'data_swap': 1 << 2,
        'data_swap_active': 1 << 3,
        'vconn_on': 1 << 12
    }

    def __init__(self, console):
        """ Console can be either usbpd, ec, or plankton_ec UART
        This object with then be used by the class which creates
        the PDConsoleUtils class to send/receive commands to UART
        """
        # save console for UART access functions
        self.console = console

    def send_pd_command(self, cmd):
        """Send command to PD console UART

        @param cmd: pd command string
        """
        self.console.send_command(cmd)

    def send_pd_command_get_output(self, cmd, regexp):
        """Send command to PD console, wait for response

        @param cmd: pd command string
        @param regexp: regular expression for desired output
        """
        return self.console.send_command_get_output(cmd, regexp)

    def verify_pd_console(self):
        """Verify that PD commands exist on UART console

        Send 'help' command to UART console
        @returns: True if 'pd' is found, False if not
        """

        l = self.console.send_command_get_output('help', ['(pd)\s+([\w]+)'])
        if l[0][1] == 'pd':
            return True
        else:
            return False

    def execute_pd_state_cmd(self, port):
        """Get PD state for specified channel

        pd 0/1 state command gives produces 5 fields. The full response
        line is captured and then parsed to extract each field to fill
        the dict containing port, polarity, role, pd_state, and flags.

        @param port: Type C PD port 0 or 1

        @returns: A dict with the 5 fields listed above
        """
        cmd = 'pd'
        subcmd = 'state'
        pd_cmd = cmd +" " + str(port) + " " + subcmd
        # Two FW versions for this command, get full line.
        m = self.send_pd_command_get_output(pd_cmd,
                                            ['(Port.*) - (Role:.*)\r'])

        # Extract desired values from result string
        state_result = {}
        for key, regexp in self.PD_STATE_DICT.iteritems():
            value = re.search(regexp, m[0][0])
            if value:
                state_result[key] = value.group(1)
            else:
                raise error.TestFail('pd 0/1 state: %r value not found' % (key))

        return state_result

    def get_pd_state(self, port):
        """Get the current PD state

        @param port: Type C PD port 0/1
        @returns: current pd state
        """

        pd_dict = self.execute_pd_state_cmd(port)
        return pd_dict['pd_state']

    def get_pd_port(self, port):
        """Get the current PD port

        @param port: Type C PD port 0/1
        @returns: current pd state
        """
        pd_dict = self.execute_pd_state_cmd(port)
        return pd_dict['port']

    def get_pd_role(self, port):
        """Get the current PD power role (source or sink)

        @param port: Type C PD port 0/1
        @returns: current pd state
        """
        pd_dict = self.execute_pd_state_cmd(port)
        return pd_dict['role']

    def get_pd_flags(self, port):
        """Get the current PD flags

        @param port: Type C PD port 0/1
        @returns: current pd state
        """
        pd_dict = self.execute_pd_state_cmd(port)
        return pd_dict['flags']

    def get_pd_dualrole(self):
        """Get the current PD dualrole setting

        @returns: current PD dualrole setting
        """
        cmd = 'pd dualrole'
        dual_list = self.send_pd_command_get_output(cmd,
                                ['dual-role toggling:\s+([\w ]+)'])
        return dual_list[0][1]

    def set_pd_dualrole(self, value):
        """Set pd dualrole

        It can be set to either:
        1. on
        2. off
        3. snk (force sink mode)
        4. src (force source mode)
        After setting, the current value is read to confirm that it
        was set properly.

        @param value: One of the 4 options listed
        """
        # Get string required for console command
        dual_index = self.dual_index[value]
        # Create console command
        cmd = 'pd dualrole ' + self.dualrole_cmd[dual_index]
        self.console.send_command(cmd)
        time.sleep(self.DUALROLE_QUERY_DELAY)
        # Get current setting to verify that command was successful
        dual = self.get_pd_dualrole()
        # If it doesn't match, then raise error
        if dual != self.dualrole_resp[dual_index]:
            raise error.TestFail("dualrole error: " +
                                 self.dualrole_resp[dual_index] + " != "+dual)

    def query_pd_connection(self):
        """Determine if PD connection is present

        Try the 'pd 0/1 state' command and see if it's in either
        expected state of a conneciton. Record the port number
        that has an active connection

        @returns: dict with params port, connect, and state
        """
        status = {}
        port = 0;
        status['connect'] = False
        status['port'] = port
        state = self.get_pd_state(port)
        # Check port 0 first
        if state == self.SRC_CONNECT or state == self.SNK_CONNECT:
            status['connect'] = True
            status['role'] = state
        else:
            port = 1
            status['port'] = port
            state = self.get_pd_state(port)
            # Check port 1
            if state == self.SRC_CONNECT or state == self.SNK_CONNECT:
                status['connect'] = True
                status['role'] = state

        return status

    def disable_pd_console_debug(self):
        """Turn off PD console debug

        """
        cmd = 'pd dump 0'
        self.send_pd_command(cmd)

    def enable_pd_console_debug(self):
        """Enable PD console debug level 1

        """
        cmd = 'pd dump 1'
        self.send_pd_command(cmd)

    def is_pd_flag_set(self, port, key):
        """Test a bit in PD protocol state flags

        The flag word contains various PD protocol state information.
        This method allows for a specific flag to be tested.

        @param port: Port which has the active PD connection
        @param key: dict key to retrieve the flag bit mapping

        @returns True if the bit to be tested is set
        """
        pd_flags = self.get_pd_flags(port)
        return bool(self.PD_STATE_FLAGS_DICT[key] & int(pd_flags, 16))

    def is_pd_connected(self, port):
        """Check if a PD connection is active

        @param port: port to be used for pd console commands

        @returns True if port is in connected state
        """
        state = self.get_pd_state(port)
        return bool(state == self.SRC_CONNECT or state == self.SNK_CONNECT)

    def is_pd_dual_role_enabled(self):
        """Check if a PD device is in dualrole mode

        @returns True is dualrole mode is active, false otherwise
        """
        drp = self.get_pd_dualrole()
        return bool(drp == self.dualrole_resp[self.dual_index['on']])


class PDConnectionUtils(PDConsoleUtils):
    """Provides a set of methods common to USB PD FAFT tests

    This Class is used for PD utility methods that require access
    to both Plankton and DUT PD consoles.

    """

    def __init__(self, dut_console, plankton_console):
        """
        @param dut_console: PD console object for DUT
        @param plankton_console: PD console object for Plankton
        """
        # save console for DUT PD UART access functions
        self.dut_console = dut_console
        # save console for Plankton UART access functions
        self.plankton_console = plankton_console
        super(PDConnectionUtils, self).__init__(dut_console)

    def _verify_plankton_connection(self, port):
        """Verify DUT to Plankton PD connection

        This method checks for a Plankton PD connection for the
        given port by first verifying if a PD connection is present.
        If found, then it uses a Plankton feature to force a PD disconnect.
        If the port is no longer in the connected state, and following
        a delay, is found to be back in the connected state, then
        a DUT pd to Plankton connection is verified.

        @param port: DUT pd port to test

        @returns True if DUT to Plankton pd connection is verified
        """
        DISCONNECT_CHECK_TIME = 0.5
        DISCONNECT_TIME_SEC = 2
        # plankton console command to force PD disconnect
        disc_cmd = 'fake_disconnect 100 %d' % (DISCONNECT_TIME_SEC * 1000)
        # Only check for Plankton if DUT has active PD connection
        if self.dut_console.is_pd_connected(port):
            # Attempt to force PD disconnection
            self.plankton_console.send_pd_command(disc_cmd)
            time.sleep(DISCONNECT_CHECK_TIME)
            # Verify that DUT PD port is no longer connected
            if self.dut_console.is_pd_connected(port) == False:
                # Wait for disconnect timer and give time to reconnect
                time.sleep(self.dut_console.CONNECT_TIME + DISCONNECT_TIME_SEC)
                if self.dut_console.is_pd_connected(port):
                    logging.info('Plankton connection verfied on port %d', port)
                    return True
            else:
                # Could have disconnected other port, allow it to reconnect
                # before exiting.
                time.sleep(self.dut_console.CONNECT_TIME + DISCONNECT_TIME_SEC)
        return False

    def find_dut_to_plankton_connection(self):
        """Find the PD port which is connected to Plankton

        @returns DUT pd port number if found, None otherwise
        """
        for port in xrange(self.dut_console.PD_MAX_PORTS):
            # Check for DUT to Plankton connection on port
            if self._verify_plankton_connection(port):
                # Plankton PD connection found so exit
                return port
        return None
