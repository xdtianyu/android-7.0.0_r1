# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import time

from autotest_lib.client.common_lib import error
from autotest_lib.server.cros.faft.firmware_test import FirmwareTest
from autotest_lib.server.cros.servo import pd_console


class firmware_PDConnect(FirmwareTest):
    """
    Servo based USB PD connect/disconnect test. This test is written
    for the DUT and requires that the DUT support dualrole (SRC or SNK)
    operation in order to force a disconnect and connect event. The test
    does not depend on the DUT acting as source or sink, either mode
    should pass.

    Pass critera is 100%  of connections resulting in successful connections

    """
    version = 1


    def initialize(self, host, cmdline_args):
        super(firmware_PDConnect, self).initialize(host, cmdline_args)
        # Only run in normal mode
        self.switcher.setup_mode('normal')
        self.usbpd.send_command("chan 0")


    def cleanup(self):
        self.usbpd.send_command("chan 0xffffffff")
        super(firmware_PDConnect, self).cleanup()


    def _test_pd_connection(self, connect_state, port):
        """Verify current pd state matches the expected value.

        The current state will be read up to 2 times. This
        may not be required, but during development testing
        instances were observed where state reads on glados
        did not give the full state string which would then
        applear to be a failure even though the type C connection
        had been made.

        @params connect_state: Expected state string
        @params port: port number <0/1> to query
        @returns: True if state matches, false otherwise
        """
        for attempts in range(1,3):
            pd_state = self.pd.get_pd_state(self.port)
            if pd_state == connect_state:
                return True
        return False


    def run_once(self):
        """Exectue disconnect/connect sequence test

        """
        # delay between test iterations
        DUALROLE_SET_DELAY = 2

        # create objects for pd utilities
        self.pd = pd_console.PDConsoleUtils(self.usbpd)

        # Make sure PD support exists in the UART console
        if self.pd.verify_pd_console() == False:
            raise error.TestFail("pd command not present on console!")

        # Enable dualrole mode
        self.pd.set_pd_dualrole('on')
        time.sleep(DUALROLE_SET_DELAY)

        # Type C connection (PD contract) should exist at this point
        connect_status = self.pd.query_pd_connection()
        if connect_status['connect'] == False:
            raise error.TestFail("pd connection not found")
        # Record port where type C connection was detected
        self.port = connect_status['port']
        # Save the SRC vs SNK state
        connect_state = connect_status['role']

        logging.info('Type C connection detected on Port %d: %r',
                     self.port, connect_state)

        # determine the dualrole command to connect/disconnect
        if  connect_state == 'SRC_READY':
            disc_cmd = 'snk'
            connect_cmd = 'src'
        else:
            disc_cmd = 'src'
            connect_cmd = 'snk'

        # counter used for successful iterations
        success = 0
        total_attempts = 100

        # Attempt connect/disconnect iterations
        for test_count in range(1, total_attempts + 1):
            logging.info ('\n************ Iteration %r ***************',
                          test_count)
            # Force Type C disconnect
            self.pd.set_pd_dualrole(disc_cmd)
            time.sleep(DUALROLE_SET_DELAY)
            # Attempt to reconnect
            self.pd.set_pd_dualrole(connect_cmd)
            time.sleep(DUALROLE_SET_DELAY)
            # Verify connection was successful
            if self._test_pd_connection(connect_state, self.port) == True:
                success += 1

        self.pd.set_pd_dualrole('on')
        logging.info ('************ Connection Stats ***************')
        logging.info ('Attempts = %d: Connections = %d', test_count, success)
        logging.info ('*********************************************')
        if success != total_attempts:
            raise error.TestFail("Attempts = " + str(total_attempts) +
                                 ': Success = ' + str(success))

