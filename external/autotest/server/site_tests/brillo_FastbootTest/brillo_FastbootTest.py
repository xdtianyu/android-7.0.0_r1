# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import common
from autotest_lib.client.common_lib import error
from autotest_lib.server import test


class brillo_FastbootTest(test.test):
    """Verify that a Brillo device can reboot into / out of fastboot."""
    version = 1

    def run_once(self, host=None):
        """Runs the test.

        @param host: A host object representing the DUT.

        @raise TestError: Something went wrong while trying to execute the test.
        @raise TestFail: The test failed.
        """
        # Make sure we're in ADB mode.
        if not host.is_up():
            raise error.TestError('Device is not in ADB mode')

        # Switch to fastboot (implies a reboot).
        try:
            host.ensure_bootloader_mode()
        except error.AutoservError as e:
            raise error.TestFail(
                    'Failed to reboot the device into fastboot: %s' % e)

        # Now reboot back into ADB.
        try:
            host.ensure_adb_mode()
        except error.AutoservError as e:
            raise error.TestFail(
                    'Failed to reboot the device back to ADB: %s' % e)
