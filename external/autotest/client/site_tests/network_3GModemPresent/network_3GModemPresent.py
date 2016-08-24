# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error


class network_3GModemPresent(test.test):
    """
    Tests that a 3G modem is available.

    The test attempts to find a shill device corresponding to a cellular modem.

    """
    version = 1

    def run_once(self, test_env):
        with test_env:
            device = test_env.shill.find_cellular_device_object()
            if not device:
                raise error.TestFail("Could not find cellular device")
