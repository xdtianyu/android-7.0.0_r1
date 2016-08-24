#!/usr/bin/python

# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import common, os
from autotest_lib.client.bin import utils

version = 1

def setup():
    """Nothing needs to be done here."""
    pass

pwd = os.getcwd()
utils.update_version(pwd + '/src', True, version, setup)
