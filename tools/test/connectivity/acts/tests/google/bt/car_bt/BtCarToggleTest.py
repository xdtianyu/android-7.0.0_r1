#/usr/bin/env python3.4
#
# Copyright (C) 2016 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License. You may obtain a copy of
# the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations under
# the License.

"""
This test is used to test basic functionality of bluetooth adapter by turning it ON/OFF.
"""

from acts.test_utils.bt.BluetoothBaseTest import BluetoothBaseTest
from acts.test_utils.bt import bt_test_utils

class BtCarToggleTest(BluetoothBaseTest):
    def setup_class(self):
        self.droid_ad = self.android_devices[0]

    def setup_test(self):
        self.log.debug(log_energy_info(self.android_devices, "Start"))
        self.droid_ad.ed.clear_all_events()

    def teardown_test(self):
        self.log.debug(log_energy_info(self.android_devices, "End"))

    def on_fail(self, test_name, begin_time):
        bt_test_utils.take_btsnoop_logs(self.android_devices, self, test_name)

    @BluetoothBaseTest.bt_test_wrap
    def test_bluetooth_reset(self):
        """Test resetting bluetooth.

        Test the integrity of resetting bluetooth on Android.

        Steps:
        1. Toggle bluetooth off.
        2. Toggle bluetooth on.

        Returns:
          Pass if True
          Fail if False
        """
        asserts.assert_true(bt_test_utils.reset_bluetooth([self.droid_ad]), "")
