#!/usr/bin/python
# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import common
from autotest_lib.client.bin import utils

version = 1

def setup(topdir):
    srcdir = os.path.join(topdir, 'src')

    # Build lansim
    os.chdir(srcdir)
    utils.make()
    utils.system('make install')
    os.chdir(topdir)

pwd = os.getcwd()
utils.update_version(os.path.join(pwd, 'src'), True, version, setup, pwd)
