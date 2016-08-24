# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

'''
Utilities for serial port communication.
'''
import glob
import os
import re

def find_tty_by_driver(driver_name):
    '''Finds the tty terminal matched to the given driver_name.'''
    candidates = glob.glob('/dev/tty*')
    for path in candidates:
        if re.search(
            driver_name,
            os.path.realpath('/sys/class/tty/%s/device/driver' %
                             os.path.basename(path))):
            return path
    return None
