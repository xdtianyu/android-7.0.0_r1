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
This test script exercises background scan test scenarios.
"""

from queue import Empty

from acts.test_utils.bt.BluetoothBaseTest import BluetoothBaseTest
from acts.test_utils.bt.BleEnum import BluetoothAdapterState
from acts.test_utils.bt.bt_test_utils import bluetooth_off
from acts.test_utils.bt.bt_test_utils import bluetooth_on
from acts.test_utils.bt.bt_test_utils import cleanup_scanners_and_advertisers
from acts.test_utils.bt.bt_test_utils import log_energy_info
from acts.test_utils.bt.bt_test_utils import generate_ble_advertise_objects
from acts.test_utils.bt.bt_test_utils import generate_ble_scan_objects
from acts.test_utils.bt.bt_test_utils import get_advanced_droid_list
from acts.test_utils.bt.bt_test_utils import scan_result


class BleBackgroundScanTest(BluetoothBaseTest):
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
        self.adv_ad = self.android_devices[1]
        if self.droid_list[1]['max_advertisements'] == 0:
            self.tests = ()
            return

    def setup_test(self):
        self.log.debug(log_energy_info(self.android_devices, "Start"))
        if (self.scn_ad.droid.bluetoothGetLeState() ==
                BluetoothAdapterState.STATE_OFF.value):
            self.scn_ad.droid.bluetoothEnableBLE()
            self.scn_ad.ed.pop_event("BleStateChangedOn")
        for a in self.android_devices:
            a.ed.clear_all_events()
        return True

    def teardown_test(self):
        self.log.debug(log_energy_info(self.android_devices, "End"))
        cleanup_scanners_and_advertisers(
            self.scn_ad, self.active_adv_callback_list, self.adv_ad,
            self.active_adv_callback_list)
        self.active_adv_callback_list = []
        self.active_scan_callback_list = []

    def _setup_generic_advertisement(self):
        adv_callback, adv_data, adv_settings = generate_ble_advertise_objects(
            self.adv_ad.droid)
        self.adv_ad.droid.bleStartBleAdvertising(adv_callback, adv_data,
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
    def test_background_scan(self):
        """Test generic background scan.

        Tests LE background scan. The goal is to find scan results even though
        Bluetooth is turned off.

        Steps:
        1. Setup an advertisement on dut1
        2. Enable LE on the Bluetooth Adapter on dut0
        3. Toggle BT off on dut1
        4. Start a LE scan on dut0
        5. Find the advertisement from dut1

        Expected Result:
        Find a advertisement from the scan instance.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Scanning, Background Scanning
        Priority: 0
        """
        import time
        self._setup_generic_advertisement()
        self.scn_ad.droid.bluetoothToggleState(False)
        try:
            self.scn_ad.ed.pop_event(bluetooth_off, self.default_timeout)
        except Empty:
            self.log.error("Bluetooth Off event not found. Expected {}".format(
                bluetooth_off))
            return False
        self.scn_ad.droid.bluetoothDisableBLE()
        try:
            self.scn_ad.ed.pop_event(bluetooth_off, self.default_timeout)
        except Empty:
            self.log.error("Bluetooth Off event not found. Expected {}".format(
                bluetooth_off))
            return False
        self.scn_ad.droid.bluetoothEnableBLE()
        try:
            self.scn_ad.ed.pop_event(bluetooth_off, self.default_timeout*2)
        except Empty:
            self.log.error("Bluetooth On event not found. Expected {}".format(
                bluetooth_on))
            return False
        filter_list, scan_settings, scan_callback = generate_ble_scan_objects(
            self.scn_ad.droid)
        self.scn_ad.droid.bleStartBleScan(filter_list, scan_settings,
                                          scan_callback)
        expected_event = scan_result.format(scan_callback)
        try:
            self.scn_ad.ed.pop_event(expected_event, self.default_timeout)
        except Empty:
            self.log.error("Scan Result event not found. Expected {}".format(expected_event))
            return False
        return True

    @BluetoothBaseTest.bt_test_wrap
    def test_background_scan_ble_disabled(self):
        """Test background LE scanning with LE disabled.

        Tests LE background scan. The goal is to find scan results even though
        Bluetooth is turned off.

        Steps:
        1. Setup an advertisement on dut1
        2. Enable LE on the Bluetooth Adapter on dut0
        3. Toggle BT off on dut1
        4. Start a LE scan on dut0
        5. Find the advertisement from dut1

        Expected Result:
        Find a advertisement from the scan instance.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Scanning, Background Scanning
        Priority: 0
        """
        self._setup_generic_advertisement()
        self.scn_ad.droid.bluetoothEnableBLE()
        self.scn_ad.droid.bluetoothToggleState(False)
        try:
            self.scn_ad.ed.pop_event(bluetooth_off, self.default_timeout)
        except Empty:
            self.log.error("Bluetooth Off event not found. Expected {}".format(
                bluetooth_off))
            return False
        filter_list, scan_settings, scan_callback = generate_ble_scan_objects(
            self.scn_ad.droid)
        try:
            self.scn_ad.droid.bleStartBleScan(filter_list, scan_settings,
                                              scan_callback)
            expected_event = scan_result.format(scan_callback)
            try:
                self.scn_ad.ed.pop_event(expected_event, self.default_timeout)
            except Empty:
                self.log.error("Scan Result event not found. Expected {}".format(expected_event))
                return False
            self.log.info("Was able to start background scan even though ble "
                          "was disabled.")
            return False
        except Exception:
            self.log.info(
                "Was not able to start a background scan as expected.")
        return True
