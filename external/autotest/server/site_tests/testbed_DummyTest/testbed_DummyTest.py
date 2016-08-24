# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import common
from autotest_lib.server import test


class testbed_DummyTest(test.test):
    """A dummy test to verify testbed can access connected Android devices."""
    version = 1

    def run_once(self, testbed=None):
        """A dummy test to verify testbed can access connected Android devices.

        Prerequisite: All connected DUTs are in ADB mode.

        @param testbed: host object representing the testbed.
        """
        self.testbed = testbed
        for device in self.testbed.get_all_hosts():
            device.run('true')
