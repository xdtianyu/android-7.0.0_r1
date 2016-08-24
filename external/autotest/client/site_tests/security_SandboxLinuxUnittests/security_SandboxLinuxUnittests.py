# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import glob
import logging
import os

from autotest_lib.client.cros import chrome_binary_test


class security_SandboxLinuxUnittests(chrome_binary_test.ChromeBinaryTest):
    """Runs sandbox_linux_unittests."""

    version = 1
    BINARY = 'sandbox_linux_unittests'
    CRASH_DIR = '/var/spool/crash'


    def run_once(self):
        self.run_chrome_test_binary(self.BINARY)
        crash_pattern = os.path.join(self.CRASH_DIR, self.BINARY + '*')
        for filename in glob.glob(crash_pattern):
            try:
                os.remove(filename)
            except OSError as ose:
                logging.warning('Could not remove crash dump: %s', ose)
