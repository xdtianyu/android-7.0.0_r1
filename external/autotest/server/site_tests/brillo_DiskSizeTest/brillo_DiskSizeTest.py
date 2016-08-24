# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import common
from autotest_lib.client.common_lib import error
from autotest_lib.server import test


_DEFAULT_PATH = '/data'
_DEFAULT_MIN_SIZE = 64 * 1024


class brillo_DiskSizeTest(test.test):
    """Verify that a Brillo device has its wifi properly configured."""
    version = 1

    def run_once(self, host=None, path=_DEFAULT_PATH,
                 min_size=_DEFAULT_MIN_SIZE):
        """Check that a given device is large enough.

        @param host: a host object representing the DUT.
        @param path: Path to device or a location within its mounted filesystem.
        @param min_size: Minimum device size in 1K blocks.

        @raise TestFail: The test failed.
        """
        try:
            df_output = host.run_output('df %s' % path).splitlines()
        except error.AutoservRunError:
            raise error.TestFail('Failed to run df')

        device, device_size = df_output[1].split()[0:2]
        if int(device_size) < int(min_size):
            raise error.TestFail(
                    'Size of device %s (%s) is less than required (%s)' %
                    (device, device_size, min_size))
