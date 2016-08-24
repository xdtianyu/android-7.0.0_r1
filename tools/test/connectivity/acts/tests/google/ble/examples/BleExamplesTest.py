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
This script shows simple examples of how to get started with bluetooth low energy testing in acts.
"""

import pprint

from acts.controllers import android_devices
from acts.test_utils.bt.BluetoothBaseTest import BluetoothBaseTest
from acts.test_utils.bt.bt_test_utils import adv_succ
from acts.test_utils.bt.bt_test_utils import scan_result
from acts.test_utils.bt.bt_test_utils import cleanup_scanners_and_advertisers
from acts.test_utils.bt.bt_test_utils import get_advanced_droid_list
from acts.test_utils.bt.bt_test_utils import reset_bluetooth


class BleExamplesTest(BluetoothBaseTest):
    default_timeout = 10
    active_scan_callback_list = []
    active_adv_callback_list = []
    scn_droid = None
    adv_droid = None

    def __init__(self, controllers):
        BluetoothBaseTest.__init__(self, controllers)
        self.droid_list = get_advanced_droid_list(self.android_devices)
        self.scn_droid, self.scn_ed = (self.android_devices[0].droid,
                                       self.android_devices[0].ed)
        self.adv_droid, self.adv_ed = (self.android_devices[1].droid,
                                       self.android_devices[1].ed)
        if self.droid_list[1]['max_advertisements'] == 0:
            self.tests = ()
            return

    def teardown_test(self):
        cleanup_scanners_and_advertisers(
            self.android_devices[0], self.active_adv_callback_list,
            self.android_devices[1], self.active_adv_callback_list)
        self.active_adv_callback_list = []
        self.active_scan_callback_list = []

    # An optional function. This overrides the default
    # on_exception in base_test. If the test throws an
    # unexpected exception, you can customise it.
    def on_exception(self, test_name, begin_time):
        self.log.debug(
            "Test {} failed. Gathering bugreport and btsnoop logs".format(
                test_name))
        android_devices.take_bug_reports(self.android_devices, test_name,
                                         begin_time)

    @BluetoothBaseTest.bt_test_wrap
    def test_bt_toggle(self):
        """
        Test that simply toggle bluetooth
        :return:
        """
        return reset_bluetooth([self.android_devices[0]])

    '''
    Start: Examples of BLE Scanning
    '''

    @BluetoothBaseTest.bt_test_wrap
    def test_start_ble_scan(self):
        """Test to demonstrate how to start an LE scan

        Test that shows the steps to start a new ble scan.

        Steps:
        1. Create a scan filter object.
        2. Create a scan setting object.
        3. Create a scan callback object.
        4. Start an LE scan using the objects created in steps 1-3.
        5. Find an advertisement with the scanner's event dispatcher.

        Expected Result:
        A generic advertisement is found.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Scanning
        Priority: 4
        """
        filter_list = self.scn_droid.bleGenFilterList()
        scan_settings = self.scn_droid.bleBuildScanSetting()
        scan_callback = self.scn_droid.bleGenScanCallback()
        self.scn_droid.bleStartBleScan(filter_list, scan_settings,
                                       scan_callback)
        self.active_scan_callback_list.append(scan_callback)
        event_name = scan_result.format(scan_callback)
        try:
            event = self.scn_ed.pop_event(event_name, self.default_timeout)
            self.log.info("Found scan result: {}".format(pprint.pformat(
                event)))
        except Exception:
            self.log.info("Didn't find any scan results.")
        return True

    '''
    End: Examples of BLE Scanning
    '''

    @BluetoothBaseTest.bt_test_wrap
    def test_start_ble_advertise(self):
        """Test to demonstrate how to start an LE advertisement

        Test that shows the steps to start a new ble scan.

        Steps:
        1. Create a advertise data object
        2. Create a advertise settings object.
        3. Create a advertise callback object.
        4. Start an LE advertising using the objects created in steps 1-3.
        5. Find the onSuccess advertisement event.

        Expected Result:
        Advertisement is successfully advertising.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising
        Priority: 4
        """
        advertise_data = self.adv_droid.bleBuildAdvertiseData()
        advertise_settings = self.adv_droid.bleBuildAdvertiseSettings()
        advertise_callback = self.adv_droid.bleGenBleAdvertiseCallback()
        self.adv_droid.bleStartBleAdvertising(
            advertise_callback, advertise_data, advertise_settings)
        self.adv_ed.pop_event(adv_succ.format(advertise_callback))
        return True
