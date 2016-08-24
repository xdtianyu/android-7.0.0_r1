# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import common
from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.server import test


_DEFAULT_MIN_VERSION = '3.10'


class brillo_KernelVersionTest(test.test):
    """Verify that a Brillo device runs a minimum kernel version."""
    version = 1

    def run_once(self, host=None, min_version=_DEFAULT_MIN_VERSION):
        """Runs the test.

        @param host: A host object representing the DUT.
        @param min_version: Minimum kernel version required.

        @raise TestError: Something went wrong while trying to execute the test.
        @raise TestFail: The test failed.
        """
        try:
            result = host.run_output('uname -r').strip()
        except error.AutoservRunError:
            raise error.TestFail('Failed to check kernel version')

        if utils.compare_versions(result, min_version) < 0:
            raise error.TestFail(
                    'Device kernel version (%s) older than required (%s)' %
                    (result, min_version))
