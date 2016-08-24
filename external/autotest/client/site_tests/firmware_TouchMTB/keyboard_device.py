# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Keyboard device module to capture keyboard events."""

import fcntl
import os
import re
import sys

sys.path.append('../../bin/input')
import input_device

import mtb

from linux_input import EV_KEY


class KeyboardDevice:
    """A class about keyboard device properties."""
    TYPE = '(keyboard|chromeos-ec-i2c|cros-ec-spi|cros-ec-i2c|cros_ec)'

    def __init__(self, device_node=None):
        self._device_info_file = '/proc/bus/input/devices'
        self.device_node = (device_node if device_node
                                        else self.get_device_node(self.TYPE))
        self.system_device = self._non_blocking_open(self.device_node)
        self._input_event = input_device.InputEvent()

    def __del__(self):
        self.system_device.close()

    def get_device_node(self, device_type):
        """Get the keyboard device node through device info file.

        Example of the keyboard device information looks like

        I: Bus=0011 Vendor=0001 Product=0001 Version=ab41
        N: Name="AT Translated Set 2 keyboard"
        P: Phys=isa0060/serio0/input0
        S: Sysfs=/devices/platform/i8042/serio0/input/input5
        U: Uniq=
        H: Handlers=sysrq kbd event5
        """
        device_node = None
        device_found = None
        device_pattern = re.compile('N: Name=.*%s' % device_type, re.I)
        event_number_pattern = re.compile('H: Handlers=.*event(\d?)', re.I)
        with open(self._device_info_file) as info:
            for line in info:
                if device_found:
                    result = event_number_pattern.search(line)
                    if result:
                        event_number = int(result.group(1))
                        device_node = '/dev/input/event%d' % event_number
                        break
                else:
                    device_found = device_pattern.search(line)
        return device_node

    def exists(self):
        """Indicate whether this device exists or not."""
        return bool(self.device_node)

    def _non_blocking_open(self, filename):
        """Open the system file in the non-blocking mode."""
        fd = open(filename)
        fcntl.fcntl(fd, fcntl.F_SETFL, os.O_NONBLOCK)
        return fd

    def _non_blocking_read(self, fd):
        """Non-blocking read on fd."""
        try:
            self._input_event.read(fd)
            return self._input_event
        except Exception:
            return None

    def get_key_press_event(self, fd):
        """Read the keyboard device node to get the key press events."""
        event = True
        # Read the device node continuously until either a key press event
        # is got or there is no more events to read.
        while event:
            event = self._non_blocking_read(fd)
            if event and event.type == EV_KEY and event.value == 1:
                return event.code
        return None
