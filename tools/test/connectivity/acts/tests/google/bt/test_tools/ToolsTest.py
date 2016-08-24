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

from acts.test_utils.bt.BluetoothBaseTest import BluetoothBaseTest
from acts.test_utils.bt.bt_test_utils import *

import time
import pprint


class ToolsTest(BluetoothBaseTest):
    tests = None
    default_timeout = 10

    def __init__(self, controllers):
        BluetoothBaseTest.__init__(self, controllers)
        self.tests = ("test_toggle_bluetooth",
                      "test_toggle_airplane_mode",
                      "test_create_10_sms",
                      "test_continuously_log_battery_stats", )

    @BluetoothBaseTest.bt_test_wrap
    def test_toggle_bluetooth(self):
        """
        Test the integrity of toggling bluetooth on and off.
        Steps:
        1. Toggle bluetooth off.
        2. Toggle bluetooth on.
        3. Repeat steps 1 and 2 one-hundred times.
        :return: boolean test_result
        """
        droid, ed = (self.android_devices[0].droid, self.android_devices[0].ed)
        n = 0
        test_result = True
        test_result_list = []
        while n < 100:
            self.log.info("Toggling bluetooth iteration {}.".format(n))
            test_result = reset_bluetooth([self.android_devices[0]])
            start_time = time.time()
            connected_devices = droid.bluetoothGetConnectedDevices()
            print(pprint.pformat(connected_devices))
            while time.time() < start_time + 10 and len(
                    connected_devices) != 1:
                time.sleep(1)
                connected_devices = droid.bluetoothGetConnectedDevices()
                print(pprint.pformat(connected_devices))
            if len(connected_devices) != 1:
                print("died at iteration {}".format(n))
                return False
            test_result_list.append(test_result)
            n += 1
        if False in test_result_list:
            return False
        return test_result

    @BluetoothBaseTest.bt_test_wrap
    def test_toggle_airplane_mode(self):
        """
        Test the integrity of toggling airplane mode on and off.
        Steps:
        1. Toggle airplane off.
        2. Toggle airplane on.
        3. Repeat steps 1 and 2 one-hundred times.
        :return: boolean test_result
        """
        droid, ed = (self.android_devices[0].droid, self.android_devices[0].ed)
        n = 0
        test_result = True
        test_result_list = []
        while n < 100:
            self.log.info("Toggling bluetooth iteration {}.".format(n))
            droid.toggleAirplaneMode(True)
            time.sleep(6)
            droid.toggleAirplaneMode(False)
            start_time = time.time()
            connected_devices = droid.bluetoothGetConnectedDevices()
            print(pprint.pformat(connected_devices))
            while time.time() < start_time + 10 and len(
                    connected_devices) != 1:
                time.sleep(1)
                connected_devices = droid.bluetoothGetConnectedDevices()
                print(pprint.pformat(connected_devices))
            if len(connected_devices) != 1:
                print("died at iteration {}".format(n))
                return False
            test_result_list.append(test_result)
            n += 1
        if False in test_result_list:
            return False
        return test_result

    @BluetoothBaseTest.bt_test_wrap
    def test_create_10_sms(self):
        phone_number = input("Enter a phone number: ")
        message_size = input("Enter message size: ")
        for _ in range(10):
            self.android_devices[0].droid.smsSendTextMessage(
                phone_number, generate_id_by_size(int(message_size)), False)
            time.sleep(3)
        return True

    @BluetoothBaseTest.bt_test_wrap
    def test_continuously_log_battery_stats(self):
        interval = input("Enter time interval to collect stats: ")
        while True:
            self.log.info(log_energy_info(
                [self.android_devices[0]], "Log_time: {}".format(time.time())))
            time.sleep(int(interval))
        return True
