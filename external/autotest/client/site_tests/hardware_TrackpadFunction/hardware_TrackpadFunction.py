#!/usr/bin/env python
#
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import re

from subprocess import Popen, PIPE

from autotest_lib.client.bin import test
from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error

# Keys for udev db
KEY_NAME = 'NAME'
KEY_DEVPATH = 'DEVPATH'
KEY_ID_INPUT_TOUCHPAD = 'ID_INPUT_TOUCHPAD'

# True equivalences
TRUE_EQ = ['1', 'on', 'true', 'yes']

# Regular expressions for cmt module related log
UNLOAD_CMT_RE = r'UnloadModule.*cmt'
USE_CMT_STRING = "Using input driver 'cmt' for '%s'"

# Path to xorg log
XORG_LOG_PATH = '/var/log/Xorg.0.log'


class hardware_TrackpadFunction(test.test):
    '''Test to make sure trackpad functions correctly'''
    version = 1

    def _udev_from_string(self, device_string):
        # Sample lines:
        # P: /devices/LNXSYSTM:00/LNXPWRBN:00
        # E: UDEV_LOG=3
        # E: DEVPATH=/devices/LNXSYSTM:00/LNXPWRBN:00
        properties = {}
        for line in device_string.split('\n'):
            _, _, val = line.partition(': ')
            args = val.partition('=')
            if args[1] != '':
                properties[args[0]] = args[2]
        return properties

    def _udevadm_export_db(self):
        p = Popen('udevadm info --export-db', shell=True, stdout=PIPE)
        output = p.communicate()[0]
        devs = output.split('\n\n')
        return [self._udev_from_string(dev) for dev in devs]

    def run_once(self):
        """Test if cmt driver is loaded correctly.
        """
        # TODO(ihf): Delete this test once all boards run freon.
        if utils.is_freon():
            return

        devices = self._udevadm_export_db()
        named_devices = [dev for dev in devices if KEY_NAME in dev]

        touchpad_devices = []
        for named_device in named_devices:
            dev_path = named_device.get(KEY_DEVPATH)
            for dev in devices:
                # Use device path prefix match to examine if a named device is
                # touchpad or not.
                #
                # Example of named device data:
                #
                # P: /devices/platform/i8042/serio4/input/input6
                # E: UDEV_LOG=3
                # E: DEVPATH=/devices/platform/i8042/serio4/input/input6
                # E: PRODUCT=11/2/7/1b1
                # E: NAME="SynPS/2 Synaptics TouchPad"
                # E: PHYS="isa0060/serio4/input0"
                #
                # Example of the data whose DEVPATH prefix matches the DEVPATH
                # above and ID_INPUT_TOUCHPAD is true.
                #
                # P: /devices/platform/i8042/serio4/input/input6/event6
                # N: input/event6
                # S: input/by-path/platform-i8042-serio-4-event-mouse
                # E: UDEV_LOG=3
                # E: DEVPATH=/devices/platform/i8042/serio4/input/input6/event6
                # E: MAJOR=13
                # E: MINOR=70
                # E: DEVNAME=/dev/input/event6
                # E: SUBSYSTEM=input
                # E: ID_INPUT=1
                # E: ID_INPUT_TOUCHPAD=1
                if (dev.get(KEY_DEVPATH, '').find(dev_path) == 0 and
                        dev.get(KEY_ID_INPUT_TOUCHPAD, '') in TRUE_EQ):
                    touchpad_devices.append(named_device)
        if touchpad_devices:
            loaded = False
            for touchpad_device in touchpad_devices:
                name = touchpad_device.get(KEY_NAME).strip('"')
                with open(XORG_LOG_PATH, 'r') as f:
                    for line in f.readlines():
                        if USE_CMT_STRING % name in line:
                            logging.info('cmt loaded: %s', line)
                            loaded = True
                        if re.search(UNLOAD_CMT_RE, line, re.I):
                            loaded = False
                            break

                if not loaded:
                    raise error.TestFail('cmt did not load for %s' % name)
        else:
            # TODO: when touchpad_devices is empty we should check the board
            # to see if it's expected.
            logging.info('no trackpad found')
