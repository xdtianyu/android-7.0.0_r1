# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import ast, logging, re, time

from autotest_lib.client.common_lib import error

# en-US key matrix (from "kb membrane pin matrix.pdf")
KEYMATRIX = {'`': (3, 1), '1': (6, 1), '2': (6, 4), '3': (6, 2), '4': (6, 3),
             '5': (3, 3), '6': (3, 6), '7': (6, 6), '8': (6, 5), '9': (6, 9),
             '0': (6, 8), '-': (3, 8), '=': (0, 8), 'q': (7, 1), 'w': (7, 4),
             'e': (7, 2), 'r': (7, 3), 't': (2, 3), 'y': (2, 6), 'u': (7, 6),
             'i': (7, 5), 'o': (7, 9), 'p': (7, 8), '[': (2, 8), ']': (2, 5),
             '\\': (3, 11), 'a': (4, 1), 's': (4, 4), 'd': (4, 2), 'f': (4, 3),
             'g': (1, 3), 'h': (1, 6), 'j': (4, 6), 'k': (4, 5), 'l': (4, 9),
             ';': (4, 8), '\'': (1, 8), 'z': (5, 1), 'x': (5, 4), 'c': (5, 2),
             'v': (5, 3), 'b': (0, 3), 'n': (0, 6), 'm': (5, 6), ',': (5, 5),
             '.': (5, 9), '/': (5, 8), ' ': (5, 11), '<right>': (6, 12),
             '<alt_r>': (0, 10), '<down>': (6, 11), '<tab>': (2, 1),
             '<f10>': (0, 4), '<shift_r>': (7, 7), '<ctrl_r>': (4, 0),
             '<esc>': (1, 1), '<backspace>': (1, 11), '<f2>': (3, 2),
             '<alt_l>': (6, 10), '<ctrl_l>': (2, 0), '<f1>': (0, 2),
             '<search>': (0, 1), '<f3>': (2, 2), '<f4>': (1, 2), '<f5>': (3, 4),
             '<f6>': (2, 4), '<f7>': (1, 4), '<f8>': (2, 9), '<f9>': (1, 9),
             '<up>': (7, 11), '<shift_l>': (5, 7), '<enter>': (4, 11),
             '<left>': (7, 12)}


# Hostevent codes, copied from:
#     ec/include/ec_commands.h
HOSTEVENT_LID_CLOSED        = 0x00000001
HOSTEVENT_LID_OPEN          = 0x00000002
HOSTEVENT_POWER_BUTTON      = 0x00000004
HOSTEVENT_AC_CONNECTED      = 0x00000008
HOSTEVENT_AC_DISCONNECTED   = 0x00000010
HOSTEVENT_BATTERY_LOW       = 0x00000020
HOSTEVENT_BATTERY_CRITICAL  = 0x00000040
HOSTEVENT_BATTERY           = 0x00000080
HOSTEVENT_THERMAL_THRESHOLD = 0x00000100
HOSTEVENT_THERMAL_OVERLOAD  = 0x00000200
HOSTEVENT_THERMAL           = 0x00000400
HOSTEVENT_USB_CHARGER       = 0x00000800
HOSTEVENT_KEY_PRESSED       = 0x00001000
HOSTEVENT_INTERFACE_READY   = 0x00002000
# Keyboard recovery combo has been pressed
HOSTEVENT_KEYBOARD_RECOVERY = 0x00004000
# Shutdown due to thermal overload
HOSTEVENT_THERMAL_SHUTDOWN  = 0x00008000
# Shutdown due to battery level too low
HOSTEVENT_BATTERY_SHUTDOWN  = 0x00010000
HOSTEVENT_INVALID           = 0x80000000


class ChromeEC(object):
    """Manages control of a Chrome EC.

    We control the Chrome EC via the UART of a Servo board. Chrome EC
    provides many interfaces to set and get its behavior via console commands.
    This class is to abstract these interfaces.
    """

    def __init__(self, servo):
        """Initialize and keep the servo object.

        Args:
          servo: A Servo object.
        """
        self._servo = servo
        self._cached_uart_regexp = None


    def set_uart_regexp(self, regexp):
        if self._cached_uart_regexp == regexp:
            return
        self._cached_uart_regexp = regexp
        self._servo.set('ec_uart_regexp', regexp)


    def send_command(self, commands):
        """Send command through UART.

        This function opens UART pty when called, and then command is sent
        through UART.

        Args:
          commands: The commands to send, either a list or a string.
        """
        self.set_uart_regexp('None')
        if isinstance(commands, list):
            try:
                self._servo.set_nocheck('ec_uart_multicmd', ';'.join(commands))
            except error.TestFail as e:
                if 'No control named' in str(e):
                    logging.warning(
                            'The servod is too old that ec_uart_multicmd '
                            'not supported. Use ec_uart_cmd instead.')
                    for command in commands:
                        self._servo.set_nocheck('ec_uart_cmd', command)
                else:
                    raise
        else:
            self._servo.set_nocheck('ec_uart_cmd', commands)


    def send_command_get_output(self, command, regexp_list):
        """Send command through UART and wait for response.

        This function waits for response message matching regular expressions.

        Args:
          command: The command sent.
          regexp_list: List of regular expressions used to match response
            message. Note, list must be ordered.

        Returns:
          List of tuples, each of which contains the entire matched string and
          all the subgroups of the match. None if not matched.
          For example:
            response of the given command:
              High temp: 37.2
              Low temp: 36.4
            regexp_list:
              ['High temp: (\d+)\.(\d+)', 'Low temp: (\d+)\.(\d+)']
            returns:
              [('High temp: 37.2', '37', '2'), ('Low temp: 36.4', '36', '4')]

        Raises:
          error.TestError: An error when the given regexp_list is not valid.
        """
        if not isinstance(regexp_list, list):
            raise error.TestError('Arugment regexp_list is not a list: %s' %
                                  str(regexp_list))

        self.set_uart_regexp(str(regexp_list))
        self._servo.set_nocheck('ec_uart_cmd', command)
        return ast.literal_eval(self._servo.get('ec_uart_cmd'))


    def key_down(self, keyname):
        """Simulate pressing a key.

        Args:
          keyname: Key name, one of the keys of KEYMATRIX.
        """
        self.send_command('kbpress %d %d 1' %
                (KEYMATRIX[keyname][1], KEYMATRIX[keyname][0]))


    def key_up(self, keyname):
        """Simulate releasing a key.

        Args:
          keyname: Key name, one of the keys of KEYMATRIX.
        """
        self.send_command('kbpress %d %d 0' %
                (KEYMATRIX[keyname][1], KEYMATRIX[keyname][0]))


    def key_press(self, keyname):
        """Press and then release a key.

        Args:
          keyname: Key name, one of the keys of KEYMATRIX.
        """
        self.send_command([
                'kbpress %d %d 1' %
                    (KEYMATRIX[keyname][1], KEYMATRIX[keyname][0]),
                'kbpress %d %d 0' %
                    (KEYMATRIX[keyname][1], KEYMATRIX[keyname][0]),
                ])


    def send_key_string_raw(self, string):
        """Send key strokes consisting of only characters.

        Args:
          string: Raw string.
        """
        for c in string:
            self.key_press(c)


    def send_key_string(self, string):
        """Send key strokes including special keys.

        Args:
          string: Character string including special keys. An example
            is "this is an<tab>example<enter>".
        """
        for m in re.finditer("(<[^>]+>)|([^<>]+)", string):
            sp, raw = m.groups()
            if raw is not None:
                self.send_key_string_raw(raw)
            else:
                self.key_press(sp)


    def reboot(self, flags=''):
        """Reboot EC with given flags.

        Args:
          flags: Optional, a space-separated string of flags passed to the
                 reboot command, including:
                   default: EC soft reboot;
                   'hard': EC hard/cold reboot;
                   'ap-off': Leave AP off after EC reboot (by default, EC turns
                             AP on after reboot if lid is open).

        Raises:
          error.TestError: If the string of flags is invalid.
        """
        for flag in flags.split():
            if flag not in ('hard', 'ap-off'):
                raise error.TestError(
                        'The flag %s of EC reboot command is invalid.' % flag)
        self.send_command("reboot %s" % flags)


    def set_flash_write_protect(self, enable):
        """Set the software write protect of EC flash.

        Args:
          enable: True to enable write protect, False to disable.
        """
        if enable:
            self.send_command("flashwp enable")
        else:
            self.send_command("flashwp disable")


    def set_hostevent(self, codes):
        """Set the EC hostevent codes.

        Args:
          codes: Hostevent codes, HOSTEVENT_*
        """
        self.send_command("hostevent set %#x" % codes)
        # Allow enough time for EC to process input and set flag.
        # See chromium:371631 for details.
        # FIXME: Stop importing time module if this hack becomes obsolete.
        time.sleep(1)
