# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import time

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import chrome
from autotest_lib.client.common_lib.cros.network import interface

_CLIENT_COMPLETE_FLAG = '/tmp/network_FirewallHolePunch'

class network_FirewallHolePunch(test.test):
    """Tests that controls an app that can open ports."""
    version = 1
    preserve_srcdir = True


    def setup(self):
        """Sets the current directory so the app can be accessed."""
        os.chdir(self.srcdir)
        self.extension_path = os.path.join(os.path.dirname(__file__),
                                           'src/tcpserver')


    def wait_for_server_command(self):
        """Waits for the server to send a command.

        @returns True if the server responds to the request; False otherwise.

        """
        for i in range(30):
            result = utils.run('ls %s' %  _CLIENT_COMPLETE_FLAG,
                               ignore_status=True)
            if result.exit_status != 0:
                return True
            time.sleep(1)
        return False


    def interpret_command(self, command, test_error):
        """Takes the string command and performs the appropriate action.

        @param command: the command string
        @param test_error: string of the test error message


        @raises TestError if the server does not set the flag or if an
                invalid command is passed.
        """
        if self.wait_for_server_command() is False:
            raise error.TestError(test_error)

        if command == 'launch app':
            self.extension = self.cr.get_extension(self.extension_path)
            self.extension.ExecuteJavaScript('tcpUI.create();')
        elif command == 'start server':
            script = str('tcpUI.startServer("%s", %d);' %
                         (self.ip_address, self.port))
            self.extension.ExecuteJavaScript(script)
        elif command == 'stop server':
            self.extension.ExecuteJavaScript('tcpUI.stopServer();')
        elif command == 'exit app':
            script = 'commandWindow.contentWindow.close();'
            self.extension.ExecuteJavaScript(script)
            self.extension.ExecuteJavaScript('close();')
        elif command == 'logout':
            self.cr.browser.Close()
        elif command == 'login':
            flag = '--enable-firewall-hole-punching'
            self.cr = chrome.Chrome(extension_paths=[self.extension_path],
                                    is_component=False,
                                    extra_browser_args=flag)
        else:
            raise error.TestError('Invalid client command passed.')

        utils.run('touch %s' % _CLIENT_COMPLETE_FLAG)


    def run_once(self, test_sequence=None, port=8888):
        """Runs the integration test."""

        # Throw if no test sequence
        if not test_sequence:
            raise error.TestError('No test sequence was passed to client.')

        # Get the IP Address of the DUT
        ethernet = interface.Interface.get_connected_ethernet_interface()
        self.ip_address = ethernet.ipv4_address
        self.port = port

        self.cr = None
        self.extension = None

        for command in test_sequence:
            self.interpret_command(command['client_command'],
                                   command['client_error'])

