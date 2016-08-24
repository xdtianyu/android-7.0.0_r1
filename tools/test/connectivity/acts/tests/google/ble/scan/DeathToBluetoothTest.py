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
"""

from queue import Empty

from acts.test_utils.bt.BluetoothBaseTest import BluetoothBaseTest
from acts.test_utils.bt.BleEnum import *
from acts.test_utils.bt.bt_test_utils import *


class DeathToBluetoothTest(BluetoothBaseTest):
    tests = None
    default_timeout = 10
    max_scan_instances = 28
    report_delay = 2000
    scan_callbacks = []
    adv_callbacks = []
    active_scan_callback_list = []
    active_adv_callback_list = []

    def __init__(self, controllers):
        BluetoothBaseTest.__init__(self, controllers)
        self.droid_list = get_advanced_droid_list(self.android_devices)
        self.scn_ad = self.android_devices[0]
        self.tests = ("test_death", )

    def teardown_test(self):
        self.active_adv_callback_list = []
        self.active_scan_callback_list = []

    def on_exception(self, test_name, begin_time):
        reset_bluetooth(self.android_devices)

    def on_fail(self, test_name, begin_time):
        reset_bluetooth(self.android_devices)

    def _setup_generic_advertisement(self):
        adv_callback, adv_data, adv_settings = generate_ble_advertise_objects(
            self.adv_droid)
        self.adv_droid.bleStartBleAdvertising(adv_callback, adv_data,
                                              adv_settings)
        self.active_adv_callback_list.append(adv_callback)

    def _verify_no_events_found(self, event_name):
        try:
            self.scn_ad.ed.pop_event(event_name, self.default_timeout)
            self.log.error("Found an event when none was expected.")
            return False
        except Empty:
            self.log.info("No scan result found as expected.")
            return True

    @BluetoothBaseTest.bt_test_wrap
    def test_death(self):
        """
        Tests ...
        Steps
        1: ...
        :return: boolean
        """
        filter_list = self.scn_ad.droid.bleGenFilterList()
        self.scn_ad.droid.bleSetScanSettingsScanMode(
            ScanSettingsScanMode.SCAN_MODE_LOW_LATENCY.value)
        self.scn_ad.droid.bleSetScanSettingsCallbackType(6)
        # self.scn_ad.droid.bleSetScanSettingsMatchMode(2) #sticky
        self.scn_ad.droid.bleSetScanSettingsMatchMode(1)  # aggresive
        self.scn_ad.droid.bleSetScanSettingsNumOfMatches(1)
        scan_settings = self.scn_ad.droid.bleBuildScanSetting()
        scan_callback = self.scn_ad.droid.bleGenScanCallback()
        self.scn_ad.droid.bleStartBleScan(filter_list, scan_settings,
                                          scan_callback)
        for _ in range(10000):
            self.scn_ad.ed.pop_event(scan_result.format(scan_callback))
        return True
