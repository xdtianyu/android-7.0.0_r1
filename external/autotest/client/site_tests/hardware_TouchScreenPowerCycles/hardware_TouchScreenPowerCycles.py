# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os
import re
import subprocess
import tempfile
import time

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.graphics import graphics_utils


class hardware_TouchScreenPowerCycles(test.test):
    """Check if there are any spurious contacts when power is cycled."""
    version = 1

    SCREEN_ON = 1
    SCREEN_OFF = 0

    def initialize(self):
        self.touch_screen_device = self._probe_touch_screen_device()
        logging.info('Touchscreen device: %s', self.touch_screen_device)
        if self.touch_screen_device is None:
            raise error.TestError('No touch screen device is found.')

        # Make sure that the screen is turned on before conducting the test.
        self._wakeup_screen()
        self.touch_screen_status = self.SCREEN_ON

    def _wakeup_screen(self):
        """Wake up the screen if it is dark."""
        graphics_utils.screen_wakeup()
        time.sleep(2)

    def _touch_screen_on(self, interval):
        """Turn the touch screen on."""
        graphics_utils.switch_screen_on(on=1)
        self.touch_screen_status = self.SCREEN_ON
        logging.info('Touchscreen is turned on')
        time.sleep(interval)

    def _touch_screen_off(self, interval):
        """Turn the touch screen off."""
        graphics_utils.switch_screen_on(on=0)
        self.touch_screen_status = self.SCREEN_OFF
        logging.info('Touchscreen is turned off')
        time.sleep(interval)

    def _probe_touch_screen_device(self):
        """Probe the touch screen device file."""
        device_info_file = '/proc/bus/input/devices'
        if not os.path.exists(device_info_file):
            return None
        with open(device_info_file) as f:
            device_info = f.read()

        touch_screen_pattern = re.compile('name=.+%s' % 'Touchscreen', re.I)
        event_pattern = re.compile('handlers=.*event(\d+)', re.I)
        found_touch_screen = False
        touch_screen_device_file = None
        for line in device_info.splitlines():
            if (not found_touch_screen and
                touch_screen_pattern.search(line) is not None):
                found_touch_screen = True
            elif found_touch_screen:
                result = event_pattern.search(line)
                if result is not None:
                    event_no = int(result.group(1))
                    device_file = '/dev/input/event%d' % event_no
                    if os.path.exists(device_file):
                        touch_screen_device_file = device_file
                    break
        return touch_screen_device_file

    def _begin_recording(self):
        """Begin a recording process."""
        record_program = 'evemu-record'
        record_cmd = '%s %s -1' % (record_program, self.touch_screen_device)
        self.event_file = tempfile.TemporaryFile()
        self.rec_proc = subprocess.Popen(record_cmd.split(),
                                         stdout=self.event_file)

    def _end_recording(self):
        """Terminate recording process, and read/close the temp event file."""
        self.rec_proc.terminate()
        self.rec_proc.wait()
        self.event_file.seek(0)
        self.events = self.event_file.readlines()
        self.event_file.close()

    def _get_timestamp(self, event):
        """Get the timestamp of an event.

        A device event looks like: "E: 1344225607.043493 0003 0036 202"
        """
        result = re.search('E:\s*(\d+(\.\d*)?|\.\d+)', event)
        timestamp = float(result.group(1)) if result else None
        return timestamp

    def _get_number_touch_contacts(self):
        """Get the number of touch contacts.

        Count ABS_MT_TRACKING_ID with a positive ID number but not -1
        For example:
            count this event:          "E: 1365999572.107771 0003 0039 405"
            do not count this event:   "E: 1365999572.107771 0003 0039 -1"
        """
        touch_pattern = re.compile('^E:.*\s*0003\s*0039\s*\d+')
        count_contacts = len(filter(touch_pattern.search, self.events))
        return count_contacts

    def run_once(self, repeated_times=5, interval=30):
        """Run through power cycles and check spurious contacts.

        @param repeated_times: the number of power on/off cycles to check.
        @param interval: the power on/off duration in seconds.

        Turn the power on for 30 seconds, and then turn it off for another
        30 seconds. Repeat it for 5 times.
        """
        count_contacts_list = []
        count_rounds = 0
        for _ in range(repeated_times):
            self._begin_recording()
            self._touch_screen_off(interval)
            self._touch_screen_on(interval)
            self._end_recording()
            count_contacts = self._get_number_touch_contacts()
            count_contacts_list.append(count_contacts)
            if count_contacts > 0:
                count_rounds += 1

        if count_rounds > 0:
            msg1 = ('Spurious contacts detected %d out of %d iterations.' %
                    (count_rounds, repeated_times))
            msg2 = 'Count of touch contacts: %s' % str(count_contacts_list)
            ave = float(sum(count_contacts_list)) / len(count_contacts_list)
            msg3 = 'Average count of touch contacts: %.2f' % ave
            raise error.TestFail('\n'.join(['', msg1, msg2, msg3]))
