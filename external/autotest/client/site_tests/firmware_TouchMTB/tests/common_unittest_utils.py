# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This module provides some utils for unit tests."""

import os
import sys


def set_paths_for_tests():
    """Set the project path and autotest input utility path for test modules."""
    pwd = os.getcwd()
    project = 'firmware_TouchMTB'
    if os.path.basename(pwd) != project:
        msg = 'Error: execute the unittests in the directory of %s!'
        print msg % project
        sys.exit(-1)
    # Append the project path
    sys.path.append(pwd)
    # Append the autotest input utility path
    sys.path.append(os.path.join(pwd, '../../bin/input/'))


def get_tests_path():
    """Get the path for unit tests."""
    return os.path.join(os.getcwd(), 'tests')


def get_tests_data_path():
    """Get the data path for unit tests."""
    return os.path.join(get_tests_path(), 'data')


def get_device_description_path():
    """Get the path for device description files."""
    return os.path.join(get_tests_path(), 'device')


def parse_tests_data(filename, gesture_dir=''):
    """Parse the unit tests data."""
    import mtb
    filepath = os.path.join(get_tests_data_path(), gesture_dir, filename)
    with open(filepath) as test_file:
        return mtb.MtbParser().parse(test_file)


def create_mocked_devices():
    """Create mocked devices of specified platforms."""
    from firmware_constants import PLATFORM
    from touch_device import TouchDevice

    description_path = get_device_description_path()
    mocked_device = {}
    for platform in PLATFORM.LIST:
        description_filename = '%s.touchpad' % platform
        description_filepath = os.path.join(description_path,
                                            description_filename)
        if not os.path.isfile(description_filepath):
            mocked_device[platform] = None
            warn_msg = 'Warning: device description file %s does not exist'
            print warn_msg % description_filepath
            continue
        mocked_device[platform] = TouchDevice(
                device_node='/dev/null',
                device_description_file=description_filepath)
    return mocked_device


set_paths_for_tests()
