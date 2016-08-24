# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, utils
from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error

class platform_KernelVersion(test.test):
    version = 1

    def run_once(self, kernel_version='2.6.31'):
        try:
            utils.check_kernel_ver(kernel_version)
        except error.TestError, e:
            logging.debug(e)
            raise error.TestFail(e)
