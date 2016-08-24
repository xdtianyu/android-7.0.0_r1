# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Touch device module provides some touch device related attributes."""

import collections
import glob
import os
import re
import tempfile

import common_util

from firmware_constants import AXIS


# Define AbsAxis class with axis attributes: min, max, and resolution
AbsAxis = collections.namedtuple('AbsAxis', ['min', 'max', 'resolution'])


class TouchDevice:
    """A class about touch device properties."""
    def __init__(self, device_node=None, is_touchscreen=False,
                 device_description_file=None):
        """If the device_description_file is provided (i.e., not None), it is
        used to create a mocked device for the purpose of replaying or
        unit tests.
        """
        self.device_node = (device_node if device_node else
                            self.get_device_node(is_touchscreen=is_touchscreen))
        self.device_description = self._get_device_description(
                device_description_file)
        self.axis_x, self.axis_y = self.parse_abs_axes()
        self.axes = {AXIS.X: self.axis_x, AXIS.Y: self.axis_y}

    @staticmethod
    def xinput_helper(cmd):
        """A helper of xinput.sh to execute a command.

        This is a method copied from factory/py/test/utils.py
        """
        dummy_script = '. /opt/google/input/xinput.sh\n%s'
        with tempfile.NamedTemporaryFile(prefix='cros_touch_xinput.') as f:
            f.write(dummy_script % cmd)
            f.flush()
            return common_util.simple_system_output('sh %s' % f.name)

    @staticmethod
    def get_device_node(is_touchscreen=False):
        """Get the touch device node through xinput.

           Touchscreens have a different device name, so this
           chooses between them.  Otherwise they are the same.

           A device id is a simple integer say 12 extracted from a string like
               Atmel maXTouch Touchscreen   id=12   [floating slave]

           A device node is extracted from "xinput list-props device_id" and
           looks like
               Device Node (250):      "/dev/input/event8"
           In this example, the device node is /dev/input/event8
        """
        device_id = TouchDevice.xinput_helper(
                'list_touchscreens' if is_touchscreen else 'list_touchpads')
        if device_id:
            device_node = TouchDevice.xinput_helper(
                    'device_get_prop %s "Device Node"' % device_id).strip('"')
        else:
            device_node = None
        return device_node

    def exists(self):
        """Indicate whether this device exists or not.

        Note that the device description is derived either from the provided
        device description file or from the system device node.
        """
        return bool(self.device_description)

    def get_dimensions_in_mm(self):
        """Get the width and height in mm of the device."""
        (left, right, top, bottom,
                resolution_x, resolution_y) = self.get_resolutions()
        width = float((right - left)) / resolution_x
        height = float((bottom - top)) / resolution_y
        return (width, height)

    def get_resolutions(self):
        """Get the resolutions in x and y axis of the device."""
        return (self.axis_x.resolution, self.axis_y.resolution)

    def get_edges(self):
        """Get the left, right, top, and bottom edges of the device."""
        return (self.axis_x.min, self.axis_x.max,
                self.axis_y.min, self.axis_y.max)

    def save_device_description_file(self, filepath, board):
        """Save the device description file in the specified filepath."""
        if self.device_description:
            # Replace the device name with the board name to reduce the risk
            # of leaking the touch device name which may be confidential.
            # Take the touchpad on link as an example:
            #   N: Atmel-maXTouch-Touchpad  would be replaced with
            #   N: link-touch-device
            name = 'N: %s-touch-device\n' % board
            try:
                with open(filepath, 'w') as fo:
                    for line in self.device_description.splitlines():
                        fo.write(name if line.startswith('N:') else line + '\n')
                return True
            except Exception as e:
                msg = 'Error: %s in getting device description from %s'
                print msg % (e, self.device_node)
        return False

    def _get_device_description(self, device_description_file):
        """Get the device description either from the specified device
        description file or from the system device node.
        """
        if device_description_file:
            # Get the device description from the device description file.
            try:
                with open(device_description_file) as dd:
                    return dd.read()
            except Exception as e:
                msg = 'Error: %s in opening the device description file: %s'
                print msg % (e, device_description_file)
        elif self.device_node:
            # Get the device description from the device node.
            cmd = 'evemu-describe %s' % self.device_node
            try:
                return common_util.simple_system_output(cmd)
            except Exception as e:
                msg = 'Error: %s in getting the device description from %s'
                print msg % (e, self.device_node)
        return None

    def parse_abs_axes(self):
        """Prase to get information about min, max, and resolution of
           ABS_X and ABS_Y

        Example of ABS_X:
                A: 00 0 1280 0 0 12
        Example of ABS_y:
                A: 01 0 1280 0 0 12
        """
        pattern = 'A:\s*%s\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)'
        pattern_x = pattern % '00'
        pattern_y = pattern % '01'
        axis_x = axis_y = None
        if self.device_description:
            for line in self.device_description.splitlines():
                if not axis_x:
                    result = re.search(pattern_x, line, re.I)
                    if result:
                        min_x = int(result.group(1))
                        max_x = int(result.group(2))
                        resolution_x = int(result.group(5))
                        axis_x = AbsAxis(min_x, max_x, resolution_x)
                if not axis_y:
                    result = re.search(pattern_y, line, re.I)
                    if result:
                        min_y = int(result.group(1))
                        max_y = int(result.group(2))
                        resolution_y = int(result.group(5))
                        axis_y = AbsAxis(min_y, max_y, resolution_y)
        return (axis_x, axis_y)

    def pixel_to_mm(self, (pixel_x, pixel_y)):
        """Convert the point coordinate from pixel to mm."""
        mm_x = float(pixel_x - self.axis_x.min) / self.axis_x.resolution
        mm_y = float(pixel_y - self.axis_y.min) / self.axis_y.resolution
        return (mm_x, mm_y)

    def pixel_to_mm_single_axis(self, value_pixel, axis):
        """Convert the coordinate from pixel to mm."""
        value_mm = float(value_pixel - axis.min) / axis.resolution
        return value_mm

    def pixel_to_mm_single_axis_by_name(self, value_pixel, axis_name):
        """Convert the coordinate from pixel to mm."""
        return self.pixel_to_mm_single_axis(value_pixel, self.axes[axis_name])

    def get_dimensions(self):
        """Get the vendor-specified dimensions of the touch device."""
        return (self.axis_x.max - self.axis_x.min,
                self.axis_y.max - self.axis_y.min)

    def get_display_geometry(self, screen_size, display_ratio):
        """Get a preferred display geometry when running the test."""
        display_ratio = 0.8
        dev_width, dev_height = self.get_dimensions()
        screen_width, screen_height = screen_size

        if 1.0 * screen_width / screen_height <= 1.0 * dev_width / dev_height:
            disp_width = int(screen_width * display_ratio)
            disp_height = int(disp_width * dev_height / dev_width)
            disp_offset_x = 0
            disp_offset_y = screen_height - disp_height
        else:
            disp_height = int(screen_height * display_ratio)
            disp_width = int(disp_height * dev_width / dev_height)
            disp_offset_x = 0
            disp_offset_y = screen_height - disp_height

        return (disp_width, disp_height, disp_offset_x, disp_offset_y)

    def _touch_input_name_re_str(self):
        pattern_str = ('touchpad', 'trackpad')
        return '(?:%s)' % '|'.join(pattern_str)

    def get_touch_input_dir(self):
        """Get touch device input directory."""
        input_root_dir = '/sys/class/input'
        input_dirs = glob.glob(os.path.join(input_root_dir, 'input*'))
        re_pattern = re.compile(self._touch_input_name_re_str(), re.I)
        for input_dir in input_dirs:
            filename = os.path.join(input_dir, 'name')
            if os.path.isfile(filename):
                with open(filename) as f:
                    for line in f:
                        if re_pattern.search(line) is not None:
                            return input_dir
        return None

    def get_firmware_version(self):
        """Probe the firmware version."""
        input_dir = self.get_touch_input_dir()
        device_dir = 'device'

        # Get the re search pattern for firmware_version file name
        fw_list = ('firmware', 'fw')
        ver_list = ('version', 'id')
        sep_list = ('_', '-')
        re_str = '%s%s%s' % ('(?:%s)' % '|'.join(fw_list),
                             '(?:%s)' % '|'.join(sep_list),
                             '(?:%s)' % '|'.join(ver_list))
        re_pattern = re.compile(re_str, re.I)

        if input_dir is not None:
            device_dir = os.path.join(input_dir, 'device', '*')
            for f in glob.glob(device_dir):
                if os.path.isfile(f) and re_pattern.search(f):
                    with open (f) as f:
                        for line in f:
                            return line.strip('\n')
        return 'unknown'
