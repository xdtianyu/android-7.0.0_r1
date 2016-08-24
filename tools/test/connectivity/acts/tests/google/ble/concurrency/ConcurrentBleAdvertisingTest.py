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
Test script to exercises different ways Ble Advertisements can run in
concurrency. This test was designed to be run in a shield box.
"""

import concurrent
import os
import time

from queue import Empty
from acts.test_utils.bt.BluetoothBaseTest import BluetoothBaseTest
from acts.test_utils.bt.BleEnum import AdvertiseSettingsAdvertiseMode
from acts.test_utils.bt.BleEnum import ScanSettingsCallbackType
from acts.test_utils.bt.BleEnum import ScanSettingsScanMode
from acts.test_utils.bt.bt_test_utils import adv_succ
from acts.test_utils.bt.bt_test_utils import generate_ble_advertise_objects
from acts.test_utils.bt.bt_test_utils import generate_ble_scan_objects
from acts.test_utils.bt.bt_test_utils import get_advanced_droid_list
from acts.test_utils.bt.bt_test_utils import reset_bluetooth
from acts.test_utils.bt.bt_test_utils import scan_result
from acts.test_utils.bt.bt_test_utils import take_btsnoop_logs


class ConcurrentBleAdvertisingTest(BluetoothBaseTest):
    default_timeout = 10
    max_advertisements = 4

    def __init__(self, controllers):
        BluetoothBaseTest.__init__(self, controllers)
        self.droid_list = get_advanced_droid_list(self.android_devices)
        self.scn_ad = self.android_devices[0]
        self.adv_ad = self.android_devices[1]
        self.max_advertisements = self.droid_list[1]['max_advertisements']
        if self.max_advertisements == 0:
            self.tests = ()
            return

    def on_fail(self, test_name, begin_time):
        self.log.debug(
            "Test {} failed. Gathering bugreport and btsnoop logs".format(
                test_name))
        take_btsnoop_logs(self.android_devices, self, test_name)
        reset_bluetooth(self.android_devices)

    def setup_test(self):
        return reset_bluetooth(self.android_devices)

    def _verify_n_advertisements(self, num_advertisements, filter_list):
        test_result = False
        address_list = []
        self.scn_ad.droid.bleSetScanSettingsCallbackType(
            ScanSettingsCallbackType.CALLBACK_TYPE_ALL_MATCHES.value)
        self.scn_ad.droid.bleSetScanSettingsScanMode(
            ScanSettingsScanMode.SCAN_MODE_LOW_LATENCY.value)
        self.adv_ad.droid.bleSetAdvertiseSettingsAdvertiseMode(
            AdvertiseSettingsAdvertiseMode.ADVERTISE_MODE_LOW_LATENCY.value)
        advertise_data = self.adv_ad.droid.bleBuildAdvertiseData()
        advertise_settings = self.adv_ad.droid.bleBuildAdvertiseSettings()
        advertise_callback_list = []
        for i in range(num_advertisements):
            advertise_callback = self.adv_ad.droid.bleGenBleAdvertiseCallback()
            advertise_callback_list.append(advertise_callback)
            self.adv_ad.droid.bleStartBleAdvertising(
                advertise_callback, advertise_data, advertise_settings)
            try:
                self.adv_ad.ed.pop_event(
                    adv_succ.format(advertise_callback), self.default_timeout)
                self.log.info("Advertisement {} started.".format(i + 1))
            except Empty as error:
                self.log.info("Advertisement {} failed to start.".format(i +
                                                                         1))
                self.log.debug("Test failed with Empty error: {}".format(
                    error))
                return False
            except concurrent.futures._base.TimeoutError as error:
                self.log.debug(
                    "Test failed, filtering callback onSuccess never occurred: "
                    "{}".format(error))
                return False
        scan_settings = self.scn_ad.droid.bleBuildScanSetting()
        scan_callback = self.scn_ad.droid.bleGenScanCallback()
        self.scn_ad.droid.bleStartBleScan(filter_list, scan_settings,
                                          scan_callback)
        start_time = time.time()
        while (start_time + self.default_timeout) > time.time():
            event = None
            try:
                event = self.scn_ad.ed.pop_event(
                    scan_result.format(scan_callback), self.default_timeout)
            except Empty as error:
                self.log.debug("Test failed with: {}".format(error))
                return test_result
            except concurrent.futures._base.TimeoutError as error:
                self.log.debug("Test failed with: {}".format(error))
                return test_result
            address = event['data']['Result']['deviceInfo']['address']
            if address not in address_list:
                address_list.append(address)
            if len(address_list) == num_advertisements:
                test_result = True
                break
        for callback in advertise_callback_list:
            self.adv_ad.droid.bleStopBleAdvertising(callback)
        self.scn_ad.droid.bleStopBleScan(scan_callback)
        return test_result

    @BluetoothBaseTest.bt_test_wrap
    def test_max_advertisements_defaults(self):
        """Testing max advertisements.

        Test that a single device can have the max advertisements
        concurrently advertising.

        Steps:
        1. Setup the scanning android device.
        2. Setup the advertiser android device.
        3. Start scanning on the max_advertisements as defined in the script.
        4. Verify that all advertisements are found.

        Expected Result:
        All advertisements should start without errors.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Concurrency
        Priority: 0
        """
        test_result = True
        filter_list = self.scn_ad.droid.bleGenFilterList()
        self.scn_ad.droid.bleBuildScanFilter(filter_list)
        test_result = self._verify_n_advertisements(self.max_advertisements,
                                                    filter_list)
        return test_result

    @BluetoothBaseTest.bt_test_wrap
    def test_max_advertisements_include_device_name_and_filter_device_name(
            self):
        """Testing max advertisement variant.

        Test that a single device can have the max advertisements
        concurrently advertising. Include the device name as a part of the filter
        and advertisement data.

        Steps:
        1. Setup the scanning android device.
        2. Setup the advertiser android device.
        3. Include device name in each advertisement.
        4. Include device name filter in the scanner.
        5. Start scanning on the max_advertisements as defined in the script.
        6. Verify that all advertisements are found.

        Expected Result:
        All advertisements should start without errors.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Concurrency
        Priority: 2
        """
        test_result = True
        self.adv_ad.droid.bleSetAdvertiseDataIncludeDeviceName(True)
        filter_list = self.scn_ad.droid.bleGenFilterList()
        self.scn_ad.droid.bleSetScanFilterDeviceName(
            self.adv_ad.droid.bluetoothGetLocalName())
        self.scn_ad.droid.bleBuildScanFilter(filter_list)
        test_result = self._verify_n_advertisements(self.max_advertisements,
                                                    filter_list)
        return test_result

    @BluetoothBaseTest.bt_test_wrap
    def test_max_advertisements_exclude_device_name_and_filter_device_name(
            self):
        """Test max advertisement variant.

        Test that a single device can have the max advertisements concurrently
        advertising. Include the device name as a part of the filter but not the
        advertisement data.

        Steps:
        1. Setup the scanning android device.
        2. Setup the advertiser android device.
        3. Include device name filter in the scanner.
        4. Start scanning on the max_advertisements as defined in the script.
        5. Verify that no advertisements are found.

        Expected Result:
        All advertisements should start without errors.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Concurrency
        Priority: 2
        """
        test_result = True
        self.adv_ad.droid.bleSetAdvertiseDataIncludeDeviceName(False)
        filter_list = self.scn_ad.droid.bleGenFilterList()
        self.scn_ad.droid.bleSetScanFilterDeviceName(
            self.adv_ad.droid.bluetoothGetLocalName())
        self.scn_ad.droid.bleBuildScanFilter(filter_list)
        test_result = self._verify_n_advertisements(self.max_advertisements,
                                                    filter_list)
        return not test_result

    @BluetoothBaseTest.bt_test_wrap
    def test_max_advertisements_with_manufacturer_data(self):
        """Test max advertisement variant.

        Test that a single device can have the max advertisements concurrently
        advertising. Include the manufacturer data as a part of the filter and
        advertisement data.

        Steps:
        1. Setup the scanning android device.
        2. Setup the advertiser android device.
        3. Include manufacturer data in each advertisement.
        4. Include manufacturer data filter in the scanner.
        5. Start scanning on the max_advertisements as defined in the script.
        6. Verify that all advertisements are found.

        Expected Result:
        All advertisements should start without errors.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Concurrency
        Priority: 2
        """
        test_result = True
        filter_list = self.scn_ad.droid.bleGenFilterList()
        self.scn_ad.droid.bleSetScanFilterManufacturerData(1, "1")
        self.scn_ad.droid.bleBuildScanFilter(filter_list)
        self.adv_ad.droid.bleAddAdvertiseDataManufacturerId(1, "1")
        test_result = self._verify_n_advertisements(self.max_advertisements,
                                                    filter_list)
        return test_result

    @BluetoothBaseTest.bt_test_wrap
    def test_max_advertisements_with_manufacturer_data_mask(self):
        """Test max advertisements variant.

        Test that a single device can have the max advertisements concurrently
        advertising. Include the manufacturer data mask as a part of the filter
        and advertisement data.

        Steps:
        1. Setup the scanning android device.
        2. Setup the advertiser android device.
        3. Include manufacturer data in each advertisement.
        4. Include manufacturer data mask filter in the scanner.
        5. Start scanning on the max_advertisements as defined in the script.
        6. Verify that all advertisements are found.

        Expected Result:
        All advertisements should start without errors.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Concurrency
        Priority: 2
        """
        test_result = True
        filter_list = self.scn_ad.droid.bleGenFilterList()
        self.scn_ad.droid.bleSetScanFilterManufacturerData(1, "1", "1")
        self.scn_ad.droid.bleBuildScanFilter(filter_list)
        self.adv_ad.droid.bleAddAdvertiseDataManufacturerId(1, "1")
        test_result = self._verify_n_advertisements(self.max_advertisements,
                                                    filter_list)
        return test_result

    @BluetoothBaseTest.bt_test_wrap
    def test_max_advertisements_with_service_data(self):
        """Test max advertisement variant.

        Test that a single device can have the max advertisements concurrently
        advertising. Include the service data as a part of the filter and
        advertisement data.

        Steps:
        1. Setup the scanning android device.
        2. Setup the advertiser android device.
        3. Include service data in each advertisement.
        4. Include service data filter in the scanner.
        5. Start scanning on the max_advertisements as defined in the script.
        6. Verify that all advertisements are found.

        Expected Result:
        All advertisements should start without errors.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Concurrency
        Priority: 2
        """
        test_result = True
        filter_list = self.scn_ad.droid.bleGenFilterList()
        self.scn_ad.droid.bleSetScanFilterServiceData(
            "0000110A-0000-1000-8000-00805F9B34FB", "11,17,80")
        self.scn_ad.droid.bleBuildScanFilter(filter_list)
        self.adv_ad.droid.bleAddAdvertiseDataServiceData(
            "0000110A-0000-1000-8000-00805F9B34FB", "11,17,80")
        test_result = self._verify_n_advertisements(self.max_advertisements,
                                                    filter_list)
        return test_result

    @BluetoothBaseTest.bt_test_wrap
    def test_max_advertisements_with_manufacturer_data_mask_and_include_device_name(
            self):
        """Test max advertisement variant.

        Test that a single device can have the max advertisements concurrently
        advertising. Include the device name and manufacturer data as a part of
        the filter and advertisement data.

        Steps:
        1. Setup the scanning android device.
        2. Setup the advertiser android device.
        3. Include device name and manufacturer data in each advertisement.
        4. Include device name and manufacturer data filter in the scanner.
        5. Start scanning on the max_advertisements as defined in the script.
        6. Verify that all advertisements are found.

        Expected Result:
        All advertisements should start without errors.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Concurrency
        Priority: 2
        """
        test_result = True
        filter_list = self.scn_ad.droid.bleGenFilterList()
        self.adv_ad.droid.bleSetAdvertiseDataIncludeDeviceName(True)
        self.scn_ad.droid.bleSetScanFilterDeviceName(
            self.adv_ad.droid.bluetoothGetLocalName())
        self.scn_ad.droid.bleSetScanFilterManufacturerData(1, "1", "1")
        self.scn_ad.droid.bleBuildScanFilter(filter_list)
        self.adv_ad.droid.bleAddAdvertiseDataManufacturerId(1, "1")
        test_result = self._verify_n_advertisements(self.max_advertisements,
                                                    filter_list)
        return test_result

    @BluetoothBaseTest.bt_test_wrap
    def test_max_advertisements_with_service_uuids(self):
        """Test max advertisement variant.

        Test that a single device can have the max advertisements concurrently
        advertising. Include the service uuid as a part of the filter and
        advertisement data.

        Steps:
        1. Setup the scanning android device.
        2. Setup the advertiser android device.
        3. Include service uuid in each advertisement.
        4. Include service uuid filter in the scanner.
        5. Start scanning on the max_advertisements as defined in the script.
        6. Verify that all advertisements are found.

        Expected Result:
        All advertisements should start without errors.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Concurrency
        Priority: 1
        """
        test_result = True
        filter_list = self.scn_ad.droid.bleGenFilterList()
        self.scn_ad.droid.bleSetScanFilterServiceUuid(
            "00000000-0000-1000-8000-00805f9b34fb")
        self.scn_ad.droid.bleBuildScanFilter(filter_list)
        self.adv_ad.droid.bleSetAdvertiseDataSetServiceUuids(
            ["00000000-0000-1000-8000-00805f9b34fb"])
        test_result = self._verify_n_advertisements(self.max_advertisements,
                                                    filter_list)
        return test_result

    @BluetoothBaseTest.bt_test_wrap
    def test_max_advertisements_with_service_uuid_and_service_mask(self):
        """Test max advertisements variant.

        Test that a single device can have the max advertisements concurrently
        advertising. Include the service mask as a part of the filter and
        advertisement data.

        Steps:
        1. Setup the scanning android device.
        2. Setup the advertiser android device.
        3. Include service uuid in each advertisement.
        4. Include service mask filter in the scanner.
        5. Start scanning on the max_advertisements as defined in the script.
        6. Verify that all advertisements are found.

        Expected Result:
        All advertisements should start without errors.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Concurrency
        Priority: 2
        """
        test_result = True
        filter_list = self.scn_ad.droid.bleGenFilterList()
        self.scn_ad.droid.bleSetScanFilterServiceUuid(
            "00000000-0000-1000-8000-00805f9b34fb",
            "00000000-0000-1000-8000-00805f9b34fb")
        self.scn_ad.droid.bleBuildScanFilter(filter_list)
        self.adv_ad.droid.bleSetAdvertiseDataSetServiceUuids(
            ["00000000-0000-1000-8000-00805f9b34fb"])
        test_result = self._verify_n_advertisements(self.max_advertisements,
                                                    filter_list)
        return test_result

    @BluetoothBaseTest.bt_test_wrap
    def test_max_advertisements_plus_one(self):
        """Test max advertisements plus one.

        Test that a single device can have the max advertisements concurrently
        advertising but fail on starting the max advertisements plus one.
        filter and advertisement data.

        Steps:
        1. Setup the scanning android device.
        2. Setup the advertiser android device.
        3. Start max_advertisements + 1.

        Expected Result:
        The last advertisement should fail.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Concurrency
        Priority: 0
        """
        test_result = True
        filter_list = self.scn_ad.droid.bleGenFilterList()
        self.scn_ad.droid.bleBuildScanFilter(filter_list)
        test_result = self._verify_n_advertisements(
            self.max_advertisements + 1, filter_list)
        return not test_result

    @BluetoothBaseTest.bt_test_wrap
    def test_start_two_advertisements_on_same_callback(self):
        """Test invalid advertisement scenario.

        Test that a single device cannot have two advertisements start on the
        same callback.

        Steps:
        1. Setup the scanning android device.
        2. Setup the advertiser android device.
        3. Call start ble advertising on the same callback.

        Expected Result:
        The second call of start advertising on the same callback should fail.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Concurrency
        Priority: 1
        """
        test_result = True
        advertise_callback, advertise_data, advertise_settings = (
            generate_ble_advertise_objects(self.adv_ad.droid))
        self.adv_ad.droid.bleStartBleAdvertising(
            advertise_callback, advertise_data, advertise_settings)
        try:
            self.adv_ad.ed.pop_event(
                adv_succ.format(advertise_callback), self.default_timeout)
        except Empty as error:
            self.log.debug("Test failed with Empty error: {}".format(error))
            return False
        except concurrent.futures._base.TimeoutError as error:
            self.log.debug(
                "Test failed, filtering callback onSuccess never occurred: {}"
                .format(error))
        try:
            self.adv_ad.droid.bleStartBleAdvertising(
                advertise_callback, advertise_data, advertise_settings)
            self.adv_ad.ed.pop_event(
                adv_succ.format(advertise_callback), self.default_timeout)
            test_result = False
        except Empty as error:
            self.log.debug("Test passed with Empty error: {}".format(error))
        except concurrent.futures._base.TimeoutError as error:
            self.log.debug(
                "Test passed, filtering callback onSuccess never occurred: {}"
                .format(error))

        return test_result

    @BluetoothBaseTest.bt_test_wrap
    def test_toggle_advertiser_bt_state(self):
        """Test forcing stopping advertisements.

        Test that a single device resets its callbacks when the bluetooth state is
        reset. There should be no advertisements.

        Steps:
        1. Setup the scanning android device.
        2. Setup the advertiser android device.
        3. Call start ble advertising.
        4. Toggle bluetooth on and off.
        5. Scan for any advertisements.

        Expected Result:
        No advertisements should be found after toggling Bluetooth on the
        advertising device.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Concurrency
        Priority: 2
        """
        test_result = True
        advertise_callback, advertise_data, advertise_settings = (
            generate_ble_advertise_objects(self.adv_ad.droid))
        self.adv_ad.droid.bleStartBleAdvertising(
            advertise_callback, advertise_data, advertise_settings)
        try:
            self.adv_ad.ed.pop_event(
                adv_succ.format(advertise_callback), self.default_timeout)
        except Empty as error:
            self.log.debug("Test failed with Empty error: {}".format(error))
            return False
        except concurrent.futures._base.TimeoutError as error:
            self.log.debug(
                "Test failed, filtering callback onSuccess never occurred: {}".format(
                    error))
        filter_list, scan_settings, scan_callback = generate_ble_scan_objects(
            self.scn_ad.droid)
        self.scn_ad.droid.bleStartBleScan(filter_list, scan_settings,
                                          scan_callback)
        try:
            self.scn_ad.ed.pop_event(
                scan_result.format(scan_callback), self.default_timeout)
        except Empty as error:
            self.log.debug("Test failed with: {}".format(error))
            return False
        except concurrent.futures._base.TimeoutError as error:
            self.log.debug("Test failed with: {}".format(error))
            return False
        self.scn_ad.droid.bleStopBleScan(scan_callback)
        test_result = reset_bluetooth([self.android_devices[1]])
        self.scn_ad.droid.bleStartBleScan(filter_list, scan_settings,
                                          scan_callback)
        if not test_result:
            return test_result
        try:
            self.scn_ad.ed.pop_event(
                scan_result.format(scan_callback), self.default_timeout)
            return False
        except Empty as error:
            self.log.debug("Test passed with: {}".format(error))
        except concurrent.futures._base.TimeoutError as error:
            self.log.debug("Test passed with: {}".format(error))
        self.scn_ad.droid.bleStopBleScan(scan_callback)
        self.adv_ad.droid.bleStopBleAdvertising(advertise_callback)
        return test_result

    @BluetoothBaseTest.bt_test_wrap
    def test_restart_advertise_callback_after_bt_toggle(self):
        """Test starting an advertisement on a cleared out callback.

        Test that a single device resets its callbacks when the bluetooth state
        is reset.

        Steps:
        1. Setup the scanning android device.
        2. Setup the advertiser android device.
        3. Call start ble advertising.
        4. Toggle bluetooth on and off.
        5. Call start ble advertising on the same callback.

        Expected Result:
        Starting an advertisement on a callback id after toggling bluetooth
        should fail.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Concurrency
        Priority: 1
        """
        test_result = True
        advertise_callback, advertise_data, advertise_settings = (
            generate_ble_advertise_objects(self.adv_ad.droid))
        self.adv_ad.droid.bleStartBleAdvertising(
            advertise_callback, advertise_data, advertise_settings)
        try:
            self.adv_ad.ed.pop_event(
                adv_succ.format(advertise_callback), self.default_timeout)
        except Empty as error:
            self.log.debug("Test failed with Empty error: {}".format(error))
            test_result = False
        except concurrent.futures._base.TimeoutError as error:
            self.log.debug(
                "Test failed, filtering callback onSuccess never occurred: {}".format(
                    error))
        test_result = reset_bluetooth([self.android_devices[1]])
        if not test_result:
            return test_result
        self.adv_ad.droid.bleStartBleAdvertising(
            advertise_callback, advertise_data, advertise_settings)
        try:
            self.adv_ad.ed.pop_event(
                adv_succ.format(advertise_callback), self.default_timeout)
        except Empty as error:
            self.log.debug("Test failed with Empty error: {}".format(error))
            test_result = False
        except concurrent.futures._base.TimeoutError as error:
            self.log.debug(
                "Test failed, filtering callback onSuccess never occurred: {}".format(
                    error))
        return test_result
