#!/usr/bin/python

# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import common, os, shutil
from autotest_lib.client.bin import utils

version = 1

def setup(topdir):
    srcdir = os.path.join(topdir, 'src')
    os.chdir(srcdir)
    utils.make()
    os.chdir(topdir)

pwd = os.getcwd()
utils.update_version(pwd + '/src', True, version, setup, pwd)
