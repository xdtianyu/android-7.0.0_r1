# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import time
from autotest_lib.server import autotest, test

LINE_STATUS_WAIT_TIME = 5


class power_RPMTest(test.test):
    """Test RPM functionality."""
    version = 1


    def initialize(self, host, verify=True):
        """
        @param host: The host to run the test on
        @param verify: True to test both on and off for the AC power and to
            check with the host whether it sees the same state
        """
        self._host = host
        self._host_at = autotest.Autotest(host)
        self._verify = verify


    def _set_power(self, power_on):
        if power_on:
            self._host.power_on()
        else:
            self._host.power_off()

        if self._verify:
            time.sleep(LINE_STATUS_WAIT_TIME)
            self._host_at.run_test('power_CheckAC', power_on=power_on)


    def run_once(self, power_sequence=[True]):
        """Run the test.i

        @param power_sequence: Sequence of values to set the power state to in
            order
        """

        for val in power_sequence:
            self._set_power(val)
