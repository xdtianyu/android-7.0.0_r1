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

import time

from acts.test_utils.bt.BluetoothBaseTest import BluetoothBaseTest
from acts.test_utils.bt.bt_test_utils import *


class BtReconnectTest(BluetoothBaseTest):
    tests = None
    default_timeout = 10

    def __init__(self, controllers):
        BluetoothBaseTest.__init__(self, controllers)
        self.tests = ("test_tool_reconnect", )

    def setup_class(self):
        return setup_multiple_devices_for_bt_test(self.android_devices)

    def setup_test(self):
        return reset_bluetooth(self.android_devices)

    def setup_test(self):
        setup_result = reset_bluetooth(self.android_devices)
        self.log.debug(log_energy_info(self.android_devices, "Start"))
        return setup_result

    def teardown_test(self):
        self.log.debug(log_energy_info(self.android_devices, "End"))
        return True

    @BluetoothBaseTest.bt_test_wrap
    def test_tool_reconnect(self):
        droid, ed = self.android_devices[0].droid, self.android_devices[0].ed
        n = 0
        test_result = True
        test_result_list = []
        sleep_time = input(
            "Assumption: Android Device is already paired.\nEnter sleep time before toggling bluetooth back on in milliseconds:")
        sleep_time_ms = int(sleep_time) / 1000
        iteration_count = input("Enter number of iterations:")
        while n < int(iteration_count):
            self.log.info("Test iteration {}.".format(n))
            test_result = True
            self.log.info("Toggling BT state off...")
            droid.bluetoothToggleState(False)
            self.log.info("Sleeping {} milliseconds".format(sleep_time))
            time.sleep(sleep_time_ms)
            self.log.info("Toggling BT state on...")
            droid.bluetoothToggleState(True)
            start_time = time.time()
            connected_devices = droid.bluetoothGetConnectedDevices()
            self.log.info(
                "Waiting up to 10 seconds for device to reconnect...")
            while time.time() < start_time + 10 and len(
                    connected_devices) != 1:
                connected_devices = droid.bluetoothGetConnectedDevices()
                if len(connected_devices) > 0:
                    break
            if len(connected_devices) != 1:
                print(
                    "Failed to reconnect at iteration {}... continuing".format(
                        n))
            test_result_list.append(test_result)
            n += 1
        if False in test_result_list:
            return False
        return test_result
