#!/usr/bin/python
#
# Copyright (c) 2010 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, os

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error


class platform_Nvram(test.test):
    """
    Test /dev/nvram
    """
    version = 1

    def run_once(self):
        nvram_path = '/dev/nvram'
        if not os.path.exists(nvram_path):
            raise error.TestFail('%s does not exist.' % nvram_path)
        if not open(nvram_path, 'rb').read(1):
            raise error.TestFail('cannot read from %s.' % nvram_path)
