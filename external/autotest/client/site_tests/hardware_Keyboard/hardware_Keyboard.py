# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import glob, logging, os, sys, commands

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error

class hardware_Keyboard(test.test):
    """
    Test the keyboard through the user mode /dev/input/event interface.
    """
    version = 1
    dev_input_event_path = '/dev/input/event*'
    supported_keys = ['Esc', 'F1', 'F2', 'F3', 'F4', 'F5','F6', 'F7', 'F8',
                      'F9', 'F10', 'Grave', 'Minus', 'Equal', 'Backspace',
                      '1', '2', '3', '4', '5', '6', '7', '8', '9', '0',
                      'Tab', 'LeftBrace', 'RightBrace', 'BackSlash',
                      'LeftMeta', 'Semicolon', 'Apostrophe', 'Enter',
                      'LeftShift', 'Comma', 'Dot', 'Slash', 'RightShift',
                      'LeftControl', 'LeftAlt', 'Space', 'RightAlt',
                      'RightCtrl', 'Up', 'Down', 'Left', 'Right',
                      'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P',
                      'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L',
                      'Z', 'X', 'C', 'V', 'B', 'N', 'M']
    live_test_key = 'LeftMeta'
    preserve_srcdir = True

    def setup(self):
        os.chdir(self.srcdir)
        utils.make()

    def _supported(self, event, key_name):
        cmd = os.path.join(self.srcdir, 'evtest') + ' ' + event
        cmd += ' -s ' + key_name
        (status, output) = commands.getstatusoutput(cmd)
        if status:
            logging.error('Unsupported Key : %s' % key_name)
            return False
        logging.info('%s : %s' % (key_name, output))
        return True

    def run_once(self):
        high_key_count = 0
        high_key_event = ''
        for event in glob.glob(hardware_Keyboard.dev_input_event_path):
            # Find the event file with the most keys
            cmd = os.path.join(self.srcdir, 'evtest') + ' ' + event
            cmd += ' -n'
            (status, output) = commands.getstatusoutput(cmd)
            if status:  ## bad event, log the command's output as a warning
                logging.warning("Bad event. cmd : %s" % cmd)
                logging.warning(output)
                continue
            num_keys = int(output)
            if (num_keys > high_key_count):
                high_key_count = num_keys
                high_key_event = event
        logging.info('Event with most is %s with %d keys' % (high_key_event,
                                                             high_key_count))
        if (high_key_count < len(hardware_Keyboard.supported_keys)):
            raise error.TestError('No suitable keyboard found.')
        # Check that all necessary keyboard keys exist.
        if not all(self._supported(high_key_event, key_name)
                   for key_name in hardware_Keyboard.supported_keys):
            raise error.TestError('Required key unsupported in %s' %
                                  high_key_event)
        # Test one live keystroke. Test will wait on user input.
        cmd = os.path.join(self.srcdir, 'evtest') + ' ' + high_key_event
        cmd += ' -k'
        (status, output) = commands.getstatusoutput(cmd)
        if status:
            raise error.TestError('Key Capture Test failed : %s' % output);
        if (output != hardware_Keyboard.live_test_key):
            raise error.TestError('Incorrect key pressed : %s' % output);
