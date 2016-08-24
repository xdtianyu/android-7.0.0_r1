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
OnLost onFound Stress Test.
"""

import threading
import time

from queue import Empty
from acts.test_utils.bt.BluetoothBaseTest import BluetoothBaseTest
from acts.test_utils.bt.BleEnum import AdvertiseSettingsAdvertiseMode
from acts.test_utils.bt.BleEnum import ScanSettingsCallbackType
from acts.test_utils.bt.BleEnum import ScanSettingsScanMode
from acts.test_utils.bt.BleEnum import ScanSettingsMatchMode
from acts.test_utils.bt.BleEnum import ScanSettingsMatchNum
from acts.test_utils.bt.bt_test_utils import cleanup_scanners_and_advertisers
from acts.test_utils.bt.bt_test_utils import get_advanced_droid_list
from acts.test_utils.bt.bt_gatt_utils import orchestrate_gatt_connection
from acts.test_utils.bt.bt_test_utils import reset_bluetooth
from acts.test_utils.bt.bt_gatt_utils import run_continuous_write_descriptor
from acts.test_utils.bt.bt_gatt_utils import setup_multiple_services


class BleOnLostOnFoundStressTest(BluetoothBaseTest):
    default_timeout = 10
    max_scan_instances = 28
    report_delay = 2000
    active_scan_callback_list = []
    active_adv_callback_list = []
    scan_result = "BleScan{}onScanResults"
    batch_scan_result = "BleScan{}onBatchScanResult"

    def __init__(self, controllers):
        BluetoothBaseTest.__init__(self, controllers)
        self.droid_list = get_advanced_droid_list(self.android_devices)
        self.scn_ad = self.android_devices[0]
        self.adv_ad = self.android_devices[1]
        if self.droid_list[1]['max_advertisements'] == 0:
            self.tests = ()
            return

    def teardown_test(self):
        cleanup_scanners_and_advertisers(
            self.scn_ad, self.active_adv_callback_list, self.scn_ad,
            self.active_adv_callback_list)
        self.active_adv_callback_list = []
        self.active_scan_callback_list = []

    def on_exception(self, test_name, begin_time):
        reset_bluetooth(self.android_devices)

    def _start_generic_advertisement_include_device_name(self):
        self.adv_ad.droid.bleSetAdvertiseDataIncludeDeviceName(True)
        self.adv_ad.droid.bleSetAdvertiseSettingsAdvertiseMode(
            AdvertiseSettingsAdvertiseMode.ADVERTISE_MODE_LOW_LATENCY.value)
        advertise_data = self.adv_ad.droid.bleBuildAdvertiseData()
        advertise_settings = self.adv_ad.droid.bleBuildAdvertiseSettings()
        advertise_callback = self.adv_ad.droid.bleGenBleAdvertiseCallback()
        self.adv_ad.droid.bleStartBleAdvertising(
            advertise_callback, advertise_data, advertise_settings)
        self.adv_ad.ed.pop_event(
            "BleAdvertise{}onSuccess".format(advertise_callback),
            self.default_timeout)
        self.active_adv_callback_list.append(advertise_callback)
        return advertise_callback

    def _verify_no_events_found(self, event_name):
        try:
            self.scn_ad.ed.pop_event(event_name, self.default_timeout)
            self.log.error("Found an event when none was expected.")
            return False
        except Empty:
            self.log.info("No scan result found as expected.")
            return True

    def _poll_energy(self):
        import random
        while True:
            self.log.debug(
                self.scn_ad.droid.bluetoothGetControllerActivityEnergyInfo(1))
            time.sleep(2)

    @BluetoothBaseTest.bt_test_wrap
    def test_on_star_while_polling_energy_stats(self):
        """
        Tests ...
        Steps
        1: ...
        :return: boolean
        """
        thread = threading.Thread(target=self._poll_energy)
        thread.start()

        filter_list = self.scn_ad.droid.bleGenFilterList()
        self.scn_ad.droid.bleSetScanFilterDeviceName(
            self.adv_ad.droid.bluetoothGetLocalName())
        self.scn_ad.droid.bleSetScanSettingsScanMode(
            ScanSettingsScanMode.SCAN_MODE_LOW_LATENCY.value)
        self.scn_ad.droid.bleSetScanSettingsCallbackType(
            ScanSettingsCallbackType.CALLBACK_TYPE_FOUND_AND_LOST.value)
        self.scn_ad.droid.bleSetScanSettingsMatchMode(
            ScanSettingsMatchMode.AGGRESIVE.value)
        self.scn_ad.droid.bleSetScanSettingsNumOfMatches(
            ScanSettingsMatchNum.MATCH_NUM_ONE_ADVERTISEMENT.value)
        scan_settings = self.scn_ad.droid.bleBuildScanSetting()
        scan_callback = self.scn_ad.droid.bleGenScanCallback()
        self.scn_ad.droid.bleBuildScanFilter(filter_list)
        self.scn_ad.droid.bleStartBleScan(filter_list, scan_settings,
                                          scan_callback)
        self.active_scan_callback_list.append(scan_callback)
        on_found_count = 0
        on_lost_count = 0
        from contextlib import suppress
        for x in range(1000):
            adv_callback = (
                self._start_generic_advertisement_include_device_name())
            with suppress(Exception):
                event = self.scn_ad.ed.pop_event(
                    self.scan_result.format(scan_callback),
                    self.default_timeout * 3)
                if event['data']['CallbackType'] == 2:
                    on_found_count += 1
                elif event['data']['CallbackType'] == 4:
                    on_lost_count += 1
            self.adv_ad.droid.bleStopBleAdvertising(adv_callback)
            with suppress(Exception):
                event2 = self.scn_ad.ed.pop_event(
                    self.scan_result.format(scan_callback),
                    self.default_timeout * 4)
                if event2['data']['CallbackType'] == 2:
                    on_found_count += 1
                elif event2['data']['CallbackType'] == 4:
                    on_lost_count += 1
        thread.join()
        return True

    @BluetoothBaseTest.bt_test_wrap
    def test_more_stress_test(self):
        gatt_server_callback, gatt_server = setup_multiple_services(
            self.adv_ad)
        bluetooth_gatt, gatt_callback, adv_callback = (
            orchestrate_gatt_connection(self.scn_ad, self.adv_ad))
        self.active_scan_callback_list.append(adv_callback)
        if self.scn_ad.droid.gattClientDiscoverServices(bluetooth_gatt):
            event = self.scn_ad.ed.pop_event(
                "GattConnect{}onServicesDiscovered".format(bluetooth_gatt),
                self.default_timeout)
            discovered_services_index = event['data']['ServicesIndex']
        else:
            self.log.info("Failed to discover services.")
            return False
        services_count = self.scn_ad.droid.gattClientGetDiscoveredServicesCount(
            discovered_services_index)
        thread = threading.Thread(
            target=run_continuous_write_descriptor,
            args=(self.scn_ad.droid, self.scn_ad.ed, self.adv_ad.droid,
                  self.adv_ad.ed, gatt_server, gatt_server_callback,
                  bluetooth_gatt, services_count, discovered_services_index))
        thread.start()
        thread2 = threading.Thread(target=self._poll_energy)
        thread2.start()

        filter_list = self.scn_ad.droid.bleGenFilterList()
        self.scn_ad.droid.bleSetScanFilterDeviceName(
            self.adv_ad.droid.bluetoothGetLocalName())
        self.scn_ad.droid.bleSetScanSettingsScanMode(
            ScanSettingsScanMode.SCAN_MODE_LOW_LATENCY.value)
        self.scn_ad.droid.bleSetScanSettingsCallbackType(
            ScanSettingsCallbackType.CALLBACK_TYPE_FOUND_AND_LOST.value)
        self.scn_ad.droid.bleSetScanSettingsMatchMode(
            ScanSettingsMatchMode.AGGRESIVE.value)
        self.scn_ad.droid.bleSetScanSettingsNumOfMatches(
            ScanSettingsMatchNum.MATCH_NUM_ONE_ADVERTISEMENT.value)
        scan_settings = self.scn_ad.droid.bleBuildScanSetting()
        scan_callback = self.scn_ad.droid.bleGenScanCallback()
        self.scn_ad.droid.bleBuildScanFilter(filter_list)
        self.scn_ad.droid.bleStartBleScan(filter_list, scan_settings,
                                          scan_callback)
        self.active_scan_callback_list.append(scan_callback)
        on_found_count = 0
        on_lost_count = 0
        time.sleep(60)
        from contextlib import suppress
        for x in range(1000):
            adv_callback = self._start_generic_advertisement_include_device_name(
            )
            with suppress(Exception):
                event = self.scn_ad.ed.pop_event(
                    self.scan_result.format(scan_callback),
                    self.default_timeout * 3)
                if event['data']['CallbackType'] == 2:
                    on_found_count += 1
                elif event['data']['CallbackType'] == 4:
                    on_lost_count += 1
            self.adv_ad.droid.bleStopBleAdvertising(adv_callback)
            with suppress(Exception):
                event2 = self.scn_ad.ed.pop_event(
                    self.scan_result.format(scan_callback),
                    self.default_timeout * 4)
                if event2['data']['CallbackType'] == 2:
                    on_found_count += 1
                elif event2['data']['CallbackType'] == 4:
                    on_lost_count += 1
        thread.join()
        thread2.join()
        return True
