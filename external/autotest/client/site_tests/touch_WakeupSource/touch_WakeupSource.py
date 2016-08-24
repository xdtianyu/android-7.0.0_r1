# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os

from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import touch_playback_test_base


class touch_WakeupSource(touch_playback_test_base.touch_playback_test_base):
    """Check that touchpad/touchscreen are set/not set as wake sources."""
    version = 1

    # Devices whose touchpads should not be a wake source.
    _NO_TOUCHPAD_WAKE = ['clapper', 'glimmer', 'veyron_minnie']

    # Devices with Synaptics touchpads that do not report wake source,
    # or reference platforms like Rambi which are broken but do not ship,
    # or devices like Cyan which don't report this way: crosbug.com/p/46019.
    _INVALID_TOUCHPADS = ['x86-alex', 'x86-alex_he', 'x86-zgb', 'x86-zgb_he',
                          'x86-mario', 'stout', 'rambi', 'cyan']
    _INVALID_TOUCHSCREENS = ['cyan', 'sumo']

    def _find_wakeup_file(self, input_type):
        """Return path to wakeup file or None.

        If the file does not exist, check the parent bus for wakeup rules
        as well, as is the setup for some devices.

        @param input_type: e.g. 'touchpad' or 'mouse'. See parent class for
                all options.

        @raises: TestError if input_type lacks required information.

        """
        device_dir = self.player.devices[input_type].device_dir
        if not device_dir:
            raise error.TestError('No device directory for %s!' % input_type)

        filename = os.path.join(device_dir, 'power', 'wakeup')
        if not os.path.isfile(filename):
            logging.info('%s not found for %s', filename, input_type)

            # Look for wakeup file on parent bus instead.
            event = self.player.devices[input_type].node.split('/')[-1]

            parent = None
            i2c_devices_dir = os.path.join('/', 'sys', 'bus', 'i2c', 'devices')
            for device_dir in os.listdir(i2c_devices_dir):
                event_search = os.path.join(i2c_devices_dir, device_dir, '*',
                                            'input', 'input*', event)
                match_count = utils.run('ls %s 2>/dev/null | wc -l' % (
                        event_search)).stdout.strip()
                if int(match_count) > 0:
                    parent = os.path.join(i2c_devices_dir, device_dir)
                    break
            if parent is None:
                logging.info('Could not find parent bus for %s.', input_type)
                return None

            logging.info('Parent bus of %s is %s.', input_type, parent)
            filename = os.path.join(parent, 'power', 'wakeup')
            if not os.path.isfile(filename):
                logging.info('%s not found either.', filename)
                return None

        return filename

    def _is_wake_source(self, input_type):
        """Return True if the given device is a wake source, else False.

        If the file does not exist, return False.

        @param input_type: e.g. 'touchpad' or 'mouse'. See parent class for
                all options.

        @raises: TestError if test cannot interpret the file contents.

        """
        filename = self._find_wakeup_file(input_type)
        if filename is None:
            return False

        result = utils.run('cat %s' % filename).stdout.strip()
        if result == 'enabled':
            logging.info('Found that %s is a wake source.', input_type)
            return True
        elif result == 'disabled':
            logging.info('Found that %s is not a wake source.', input_type)
            return False
        raise error.TestError('Wakeup file for %s said "%s".' %
                              (input_type, result))

    def run_once(self):
        """Entry point of this test."""
        # Check that touchpad is a wake source for all but the excepted boards.
        if (self._has_touchpad and
            self._platform not in self._INVALID_TOUCHPADS):
            if self._platform in self._NO_TOUCHPAD_WAKE:
                if self._is_wake_source('touchpad'):
                    raise error.TestFail('Touchpad is a wake source!')
            else:
                if not self._is_wake_source('touchpad'):
                    raise error.TestFail('Touchpad is not a wake source!')

        # Check that touchscreen is not a wake source (if present).
        # Devices without a touchpad should have touchscreen as wake source.
        if (self._has_touchscreen and
            self._platform not in self._INVALID_TOUCHSCREENS):
            touchscreen_wake = self._is_wake_source('touchscreen')
            if self._has_touchpad and touchscreen_wake:
                raise error.TestFail('Touchscreen is a wake source!')
            if not self._has_touchpad and not touchscreen_wake:
                raise error.TestFail('Touchscreen is not a wake source!')

