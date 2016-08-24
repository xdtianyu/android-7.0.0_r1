# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import glob
import logging
import pprint
from threading import Timer

from autotest_lib.client.bin.input.input_device import *

class firmwareCheckKeys(object):
    version = 1
    actual_output = []
    device = None
    ev = None

    def __init__(self):
        for evdev in glob.glob("/dev/input/event*"):
            device = InputDevice(evdev)
            if device.is_keyboard():
                print 'keyboard device %s' % evdev
                self.device = device

    def _keyboard_input(self):
        """Read key presses."""
        index = 0
        while True:
            self.ev.read(self.device.f)
            if self.ev.code != KEY_RESERVED:
                print "EventCode is %d value is %d" % (self.ev.code, self.ev.value)
                if self.ev.type == 0 or self.ev.type == 1:
                    self.actual_output.append(self.ev.code)
                    index = index + 1

    def check_keys(self, expected_sequence):
        """Wait for key press for 10 seconds.

        @return number of input keys captured, -1 for error.
        """
        if not self.device:
            logging.error("Could not find a keyboard device")
            return -1

        self.ev = InputEvent()
        Timer(0, self._keyboard_input).start()

        time.sleep(10)

        # Keypresses will have a tendency to repeat as there is delay between
        # the down and up events.  We're not interested in precisely how many
        # repeats of the key there is, just what is the sequence of keys,
        # so, we will make the list unique.
        uniq_actual_output = sorted(list(set(self.actual_output)))
        if uniq_actual_output != expected_sequence:
            print 'Keys mismatched %s' % pprint.pformat(uniq_actual_output)
            return -1
        print 'Key match expected: %s' % pprint.pformat(uniq_actual_output)
        return len(uniq_actual_output)

