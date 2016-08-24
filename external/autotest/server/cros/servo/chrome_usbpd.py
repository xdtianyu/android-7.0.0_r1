# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import ast, logging

from autotest_lib.client.common_lib import error
from autotest_lib.server.cros.servo import chrome_ec


class ChromeUSBPD(chrome_ec.ChromeEC):
    """Manages control of a Chrome PD console.

    We control the Chrome USBPD via the UART of a Servo board. Chrome USBPD
    provides many interfaces to set and get its behavior via console commands.
    This class is to abstract these interfaces. The methods contained here
    differ from the ChromeEC class in that they spell out the usbpd_uart_
    instead of ec_uart_
    """

    def __init__(self, servo):
        """Initialize and keep the servo object.

        @param servo: A Servo object
        """
        super(ChromeUSBPD, self).__init__(servo)


    def set_uart_regexp(self, regexp):
        """Sets the regural expression

        @param regexp: regular expression
        """
        if self._cached_uart_regexp == regexp:
            return
        self._cached_uart_regexp = regexp
        self._servo.set('usbpd_uart_regexp', regexp)


    def send_command(self, commands):
        """Send command through UART.

        This function opens UART pty when called, and then command is sent
        through UART.

        @param commands: The commands to send, either a list or a string.
        """
        self.set_uart_regexp('None')
        if isinstance(commands, list):
            try:
                self._servo.set_nocheck('usbpd_uart_multicmd', ';'.join(commands))
            except error.TestFail as e:
                if 'No control named' in str(e):
                    logging.warning(
                            'The servod is too old that ec_uart_multicmd '
                            'not supported. Use ec_uart_cmd instead.')
                    for command in commands:
                        self._servo.set_nocheck('usbpd_uart_cmd', command)
                else:
                    raise
        else:
            self._servo.set_nocheck('usbpd_uart_cmd', commands)


    def send_command_get_output(self, command, regexp_list):
        """Send command through UART and wait for response.

        This function waits for response message matching regular expressions.

        @param command: The command sent.
        @param regexp_list: List of regular expressions used to match response
            message. Note, list must be ordered.

        @returns: List of tuples, each of which contains the entire matched
        string and all the subgroups of the match. None if not matched.
          For example:
            response of the given command:
              High temp: 37.2
              Low temp: 36.4
            regexp_list:
              ['High temp: (\d+)\.(\d+)', 'Low temp: (\d+)\.(\d+)']
            returns:
              [('High temp: 37.2', '37', '2'), ('Low temp: 36.4', '36', '4')]

        @raises  error.TestError: An error when the given regexp_list
        is not valid.
        """
        if not isinstance(regexp_list, list):
            raise error.TestError('Arugment regexp_list is not a list: %s' %
                                  str(regexp_list))

        self.set_uart_regexp(str(regexp_list))
        self._servo.set_nocheck('usbpd_uart_cmd', command)
        return ast.literal_eval(self._servo.get('usbpd_uart_cmd'))


