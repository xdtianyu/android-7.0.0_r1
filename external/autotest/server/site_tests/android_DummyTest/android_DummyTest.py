# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import common
from autotest_lib.server import test


class android_DummyTest(test.test):
    """A dummy test to verify Android device can be accessible with adb."""
    version = 1

    def run_once(self, host=None):
        """A dummy test to verify Android device can be accessible with adb.

        Prerequisite: The DUT is in ADB mode.

        @param host: host object representing the device under test.
        """
        self.host = host
        self.host.adb_run('shell ls')
