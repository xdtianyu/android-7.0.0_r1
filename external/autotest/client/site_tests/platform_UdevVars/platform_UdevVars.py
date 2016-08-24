# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os
import pyudev
import re

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
from collections import defaultdict
from operator import attrgetter

def natural_key(string_):
    """
    Derive key for natural sorting.
    @param string_: String to derive sort key for.
    From http://stackoverflow.com/questions/34518/natural-sorting-algorithm.
    """
    return [int(s) if s.isdigit() else s for s in re.split(r'(\d+)', string_)]


class platform_UdevVars(test.test):
    """Verify ChromeOS-specific udev variables."""
    version = 1


    def _input_devices(self):
        """Obtain a list of all /dev/input/event* udev devices."""
        devices = self.udev.list_devices(subsystem='input')
        # only consider the event devices
        devices = filter(attrgetter('device_node'), devices)
        devices = sorted(devices, key=lambda device: natural_key(device.device_node))
        return devices


    def _get_roles(self):
        """Get information on input devices and roles from udev."""
        self.devices_with_role = defaultdict(list)

        logging.debug('Input devices:')
        for device in self._input_devices():
            name = device.parent.attributes.get('name', '')
            logging.debug('  %s [%s]', device.device_node, name)
            role = device.get('POWERD_ROLE', None)
            if role:
                logging.debug('    POWERD_ROLE=%s', role)
                self.devices_with_role[role].append(device)


    def _dump_roles(self):
        """Log devices grouped by role for easier debugging."""
        logging.info('Roles:')
        for role in sorted(self.devices_with_role.keys()):
            for device in self.devices_with_role[role]:
                path = device.device_node
                name = device.parent.attributes.get('name', '')
                logging.info('  %-21s %s [%s]', role + ':', path, name)


    def _dump_udev_attrs(self):
        """Log udev attributes for selected devices to the debug directory."""
        for device in self._input_devices():
            devname = os.path.basename(device.device_node)

            outfile = os.path.join(self.debugdir, "udevattrs.%s" % devname)
            utils.system('udevadm info --attribute-walk --path=%s > %s' % (
                    device.sys_path, outfile))

            outfile = os.path.join(self.debugdir, "udevprops.%s" % devname)
            utils.system('udevadm info --query=property --path=%s > %s' % (
                    device.sys_path, outfile))


    def _verify_roles(self):
        """Verify that POWERD_ROLE was set on devices as expected."""

        # TODO(chromium:410968): Consider moving this to USE flags instead of
        # listing devices here.
        boards_with_touchscreen = ['link', 'samus']
        boards_maybe_touchscreen = ['rambi', 'peppy', 'glimmer', 'clapper',
                                    'nyan_big', 'nyan_blaze', 'expresso']
        boards_chromebox = ['beltino', 'guado', 'mccloud', 'panther', 'rikku', 
                            'stumpy', 'tidus', 'tricky', 'zako']
        boards_aio = ['nyan_kitty', 'tiny', 'anglar', 'monroe']

        expect_keyboard = None
        expect_touchpad = None
        expect_touchscreen = None

        board = utils.get_board()
        if board in boards_chromebox or board in boards_aio:
            expect_keyboard = [0]
            expect_touchpad = [0]
        else:
            expect_keyboard = [1]
            expect_touchpad = [1]

        if board in boards_with_touchscreen:
            expect_touchscreen = [1]
        elif board in boards_maybe_touchscreen:
            expect_touchscreen = [0, 1]
        else:
            expect_touchscreen = [0]

        expected_num_per_role = [
                ('internal_keyboard', expect_keyboard),
                ('internal_touchpad', expect_touchpad),
                ('internal_touchscreen', expect_touchscreen),
            ]

        for role, expected_num in expected_num_per_role:
            num = len(self.devices_with_role[role])
            if num not in expected_num:
                self.errors += 1
                logging.error('POWERD_ROLE=%s is present %d times, expected '
                              'one of %s', role, num, repr(expected_num))

        if len(self.devices_with_role['external_input']) != 0:
            logging.warn('%d external input devices detected',
                         len(self.devices_with_role['external_input']))


    def initialize(self):
        self.udev = pyudev.Context()


    def run_once(self):
        """
        Check that udev variables are assigned correctly by udev rules. In
        particular, verifies that powerd tags are set correctly.
        """
        logging.debug('Board: %s', utils.get_board())
        self._get_roles()
        self._dump_roles()
        self._dump_udev_attrs()

        self.errors = 0
        self._verify_roles()

        if self.errors != 0:
            raise error.TestFail('Verification of udev variables failed; see '
                                 'logs for details')
