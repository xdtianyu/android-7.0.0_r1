# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import pprint
import socket
import time

from autotest_lib.client.common_lib import error
from autotest_lib.server.cros import stress
from autotest_lib.server import autotest
from autotest_lib.server import test

_CLIENT_COMPLETE_FLAG = '/tmp/network_FirewallHolePunch'

class network_FirewallHolePunchServer(test.test):
    """Server test half of the FirewallHolePunch test."""
    version = 1


    def connect_to_dut(self):
        """Attempts to connect to the DUT

        @returns True if connection was successful; False otherwise.

        """
        clientsocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        clientsocket.settimeout(5)

        connected = False
        try:
            clientsocket.connect((self.hostname, self.port))
            connected = True
            logging.debug('Connected to client')
        except socket.timeout:
            logging.debug('Socket connection to DUT failed.')

        return connected


    def wait_for_client_test(self):
        """Waits for the client test to complete it's task.

        @returns True if the client responds to the request; False otherwise.

        """

        for i in range(30):
            result = self.client.run('ls %s' %  _CLIENT_COMPLETE_FLAG,
                                     ignore_status=True)
            if result.exit_status == 0:
                return True
            time.sleep(1)
        return False


    def functional_test(self, test_error, test_fail, connected):
        """Performs a functional testing of the firewall.

        This performs a single test while coordinating with the client test.

        @param test_error: string of the test error message
        @param test_fail: string of the test fail message
        @param connected: boolean test if the connection attempt should have
                          passed or failed.

        @raises: TestError if the client flag was not updated
        @raises: TestFail if the connection expection is not met

        """

        self.client.run('rm %s' %  _CLIENT_COMPLETE_FLAG, ignore_status=True)
        if self.wait_for_client_test() is False:
            raise error.TestError(test_error)
        if self.connect_to_dut() is connected:
            raise error.TestFail(test_fail)


    def perform_tests(self):
        """Performs all of the tests in the script."""

        for test in self.tests:
            logging.debug('Performing...')
            logging.debug(pprint.pprint(test))

            self.functional_test(test['server_error'],
                                 test['server_fail'],
                                 test['server_connected'])


    def run_once(self, host, port=8888):
        """Run the test.

        @param host: the host object
        @param port: integer value for the port the client to listen on

        """

        # Strict ordering matters here.  If an invalid order is given
        # below an exception will be thrown in the client test.
        self.tests = [# Login, fail to connect
            {'server_error': 'The client test did not login',
             'server_fail' : 'Server was able to connect (login).',
             'server_connected' : True,
             'client_command' : 'login',
             'client_error': 'Did not receive command to login (login)'
            },
            # Launch App, fail to connect
            {'server_error': 'The client test did not launch the app',
             'server_fail' : 'Server was able to connect (setup).',
             'server_connected' : True,
             'client_command' : 'launch app',
             'client_error': 'Did not receive command to launch app (setup)'
            },
            # Start server, connect
            {'server_error': 'The client test did not open the port. (1)',
             'server_fail' : 'Server was unable to connect (1).',
             'server_connected' : False,
             'client_command' : 'start server',
             'client_error': 'Did not receive command to start server (1)'
            },
            # Stop server, fail to connect
            {'server_error' : 'The client test did not close the port',
             'server_fail' : str('Server was able to connect to the port. (1) '
                                '(It should not have been able to do so.)'),
             'server_connected' : True,
             'client_command' : 'stop server',
             'client_error' : 'Did not receive command to stop server'
            },
            # Start server, connect
            {'server_error' : 'The client test did not open the port. (2)',
             'server_fail'  : 'Server was unable to connect (2).',
             'server_connected'  : False,
             'client_command' : 'start server',
             'client_error' : 'Did not receive command to start server (2)'
            },
            # Quit app, fail to connect
            {'server_error' : 'The client test did not close the app.',
             'server_fail'  : str('Server was able to connect to the port (2). '
                                '(It should not have been able to do so.)'),
             'server_connected'  : True,
             'client_command' : 'exit app',
             'client_error' : 'Did not receive command to close app.'
            },
            # Telemetry cannot relaunch a closed extension; logout and back in.
            # Logout, fail to connect
            {'server_error' : 'The client test did not quit',
             'server_fail' : str('Server was able to connect to the port (3). '
                                '(It should not have been able to do so.)'),
             'server_connected' : True,
             'client_command' : 'logout',
             'client_error': 'Did not receive command to exit.'
            },
            # Login, fail to connect
            {'server_error': 'The client test did not login',
             'server_fail' : 'Server was able to connect (login).',
             'server_connected' : True,
             'client_command' : 'login',
             'client_error': 'Did not receive command to login (login)'
            },
            # Launch app, fail to connect
            {'server_error': 'The client test did not launch the app',
             'server_fail' : 'Server was able to connect (setup2).',
             'server_connected' : True,
             'client_command' : 'launch app',
             'client_error': 'Did not receive command to launch app (setup2)'
            },
            # Start server, connect
            {'server_error': 'The client test did not open the port. (1)',
             'server_fail' : 'Server was unable to connect (1).',
             'server_connected' : False,
             'client_command' : 'start server',
             'client_error': 'Did not receive command to start server (1)'
            },
            # Logout, fail to connect
            {'server_error' : 'The client test did not quit',
             'server_fail' : str('Server was able to connect to the port (3). '
                                '(It should not have been able to do so.)'),
             'server_connected' : True,
             'client_command' : 'logout',
             'client_error': 'Did not receive command to exit.'
            }
            ]

        self.client = host
        self.hostname = self.client.hostname
        self.port = port
        client_at = autotest.Autotest(self.client)

        self.client.run('rm %s' %  _CLIENT_COMPLETE_FLAG, ignore_status=True)

        stressor = stress.CountedStressor(self.perform_tests)
        stressor.start(1)
        client_at.run_test('network_FirewallHolePunch',
                           test_sequence=self.tests,
                           port=self.port)

