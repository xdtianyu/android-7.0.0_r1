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
This test script exercises different filters and outcomes not exercised in
FilteringTest.
"""

import concurrent
import pprint
import time

from queue import Empty
from acts.test_utils.bt.BluetoothBaseTest import BluetoothBaseTest
from acts.test_utils.bt.BleEnum import AdvertiseSettingsAdvertiseMode
from acts.test_utils.bt.BleEnum import ScanSettingsScanMode
from acts.test_utils.bt.bt_test_utils import generate_ble_advertise_objects
from acts.test_utils.bt.bt_test_utils import generate_ble_scan_objects
from acts.test_utils.bt.bt_test_utils import get_advanced_droid_list
from acts.test_utils.bt.bt_test_utils import batch_scan_result
from acts.test_utils.bt.bt_test_utils import scan_result


class UniqueFilteringTest(BluetoothBaseTest):
    default_timeout = 10

    def __init__(self, controllers):
        BluetoothBaseTest.__init__(self, controllers)
        self.droid_list = get_advanced_droid_list(self.android_devices)
        self.scn_ad = self.android_devices[0]
        self.adv_ad = self.android_devices[1]
        if self.droid_list[1]['max_advertisements'] == 0:
            self.tests = ()
            return

    def blescan_verify_onfailure_event_handler(self, event):
        self.log.debug("Verifying onFailure event")
        self.log.debug(pprint.pformat(event))
        return event

    def blescan_verify_onscanresult_event_handler(self,
                                                  event,
                                                  expected_callbacktype=None,
                                                  system_time_nanos=None):
        test_result = True
        self.log.debug("Verifying onScanResult event")
        self.log.debug(pprint.pformat(event))
        callbacktype = event['data']['CallbackType']
        if callbacktype != expected_callbacktype:
            self.log.debug(
                "Expected callback type: {}, Found callback type: {}"
                .format(expected_callbacktype, callbacktype))
            test_result = False
        return test_result

    def blescan_get_mac_address_event_handler(self, event):
        return event['data']['Result']['deviceInfo']['address']

    def blescan_verify_onbatchscanresult_event_handler(
            self,
            event,
            system_time_nanos=None,
            report_delay_nanos=None):
        test_result = True
        self.log.debug("Verifying onBatchScanResult event")
        self.log.debug(pprint.pformat(event))
        for result in event['data']['Results']:
            timestamp_nanos = result['timestampNanos']
            length_of_time = timestamp_nanos - system_time_nanos
            self.log.debug("Difference in time in between scan start and "
                           "onBatchScanResult: {}".format(length_of_time))
            buffer = 1000000000  # 1 second
            if length_of_time > (report_delay_nanos + buffer):
                self.log.debug(
                    "Difference was greater than the allowable difference.")
                test_result = False
        return test_result

    @BluetoothBaseTest.bt_test_wrap
    def test_scan_flush_pending_scan_results(self):
        """Test LE scan api flush pending results.

        Test that flush pending scan results doesn't affect onScanResults from
        triggering.

        Steps:
        1. Setup the scanning android device.
        2. Setup the advertiser android devices.
        3. Trigger bluetoothFlushPendingScanResults on the scanning droid.
        4. Verify that only one onScanResults callback was triggered.

        Expected Result:
        After flushing pending scan results, make sure only one onScanResult
        callback was triggered.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 1
        """
        test_result = True
        self.adv_ad.droid.bleSetAdvertiseSettingsAdvertiseMode(
            AdvertiseSettingsAdvertiseMode.ADVERTISE_MODE_LOW_LATENCY.value)
        filter_list, scan_settings, scan_callback = generate_ble_scan_objects(
            self.scn_ad.droid)
        expected_event_name = scan_result.format(scan_callback)
        advertise_callback, advertise_data, advertise_settings = (
            generate_ble_advertise_objects(self.adv_ad.droid))
        self.adv_ad.droid.bleStartBleAdvertising(
            advertise_callback, advertise_data, advertise_settings)
        self.scn_ad.droid.bleStartBleScan(filter_list, scan_settings,
                                          scan_callback)
        self.scn_ad.droid.bleFlushPendingScanResults(scan_callback)
        worker = self.scn_ad.ed.handle_event(
            self.blescan_verify_onscanresult_event_handler,
            expected_event_name, ([1]), self.default_timeout)
        try:
            self.log.debug(worker.result(self.default_timeout))
        except Empty as error:
            test_result = False
            self.log.debug("Test failed with Empty error: {}".format(error))
        except concurrent.futures._base.TimeoutError as error:
            test_result = False
            self.log.debug("Test failed with TimeoutError: {}".format(error))
        self.scn_ad.droid.bleStopBleScan(scan_callback)
        self.adv_ad.droid.bleStopBleAdvertising(advertise_callback)
        return test_result

    @BluetoothBaseTest.bt_test_wrap
    def test_scan_trigger_on_batch_scan_results(self):
        """Test triggering batch scan results.

        Test that triggers onBatchScanResults and verifies the time to trigger
        within one second leeway.

        Steps:
        1. Setup the scanning android device with report delay seconds set to
        5000.
        2. Setup the advertiser android devices.
        3. Verify that only one onBatchScanResult callback was triggered.
        4. Compare the system time that the scan was started with the elapsed
        time that is in the callback.

        Expected Result:
        The scan event dispatcher should find an onBatchScanResult event.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning, Batch Scanning
        Priority: 2
        """
        test_result = True
        self.scn_ad.droid.bleSetScanSettingsReportDelayMillis(5000)
        filter_list, scan_settings, scan_callback = generate_ble_scan_objects(
            self.scn_ad.droid)
        expected_event_name = batch_scan_result.format(scan_callback)
        self.adv_ad.droid.bleSetAdvertiseDataIncludeDeviceName(True)
        self.adv_ad.droid.bleSetAdvertiseDataIncludeTxPowerLevel(True)
        advertise_callback, advertise_data, advertise_settings = (
            generate_ble_advertise_objects(self.adv_ad.droid))
        self.adv_ad.droid.bleStartBleAdvertising(
            advertise_callback, advertise_data, advertise_settings)
        self.scn_ad.droid.bleStartBleScan(filter_list, scan_settings,
                                          scan_callback)
        system_time_nanos = self.scn_ad.droid.getSystemElapsedRealtimeNanos()
        self.log.debug("Current system time: {}".format(system_time_nanos))
        worker = self.scn_ad.ed.handle_event(
            self.blescan_verify_onbatchscanresult_event_handler,
            expected_event_name, ([system_time_nanos, 5000000000]),
            self.default_timeout)
        try:
            self.log.debug(worker.result(self.default_timeout))
        except Empty as error:
            test_result = False
            self.log.debug("Test failed with: {}".format(error))
        except concurrent.futures._base.TimeoutError as error:
            test_result = False
            self.log.debug("Test failed with: {}".format(error))
        self.scn_ad.droid.bleStopBleScan(scan_callback)
        self.adv_ad.droid.bleStopBleAdvertising(advertise_callback)
        return test_result

    @BluetoothBaseTest.bt_test_wrap
    def test_scan_flush_results_without_on_batch_scan_results_triggered(self):
        """Test that doesn't expect a batch scan result.

        Test flush pending scan results with a report delay seconds set to 0.
        No onBatchScanResults callback should be triggered.

        Steps:
        1. Setup the scanning android device with report delay seconds set to 0
        (or just use default).
        2. Setup the advertiser android devices.

        Expected Result:
        Verify that no onBatchScanResults were triggered.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning, Batch Scanning
        Priority: 2
        """
        test_result = True
        filter_list, scan_settings, scan_callback = generate_ble_scan_objects(
            self.scn_ad.droid)
        expected_event_name = batch_scan_result.format(scan_callback)
        advertise_callback, advertise_data, advertise_settings = (
            generate_ble_advertise_objects(self.adv_ad.droid))
        self.adv_ad.droid.bleStartBleAdvertising(
            advertise_callback, advertise_data, advertise_settings)
        self.scn_ad.droid.bleStartBleScan(filter_list, scan_settings,
                                          scan_callback)
        worker = self.scn_ad.ed.handle_event(
            self.blescan_verify_onbatchscanresult_event_handler,
            expected_event_name, ([]), self.default_timeout)
        self.scn_ad.droid.bleFlushPendingScanResults(scan_callback)
        try:
            event_info = self.scn_ad.ed.pop_event(expected_event_name, 10)
            self.log.debug("Unexpectedly found an advertiser: {}".format(
                event_info))
            test_result = False
        except Empty:
            self.log.debug("No {} events were found as expected.".format(
                batch_scan_result))
        self.scn_ad.droid.bleStopBleScan(scan_callback)
        self.adv_ad.droid.bleStopBleAdvertising(advertise_callback)
        return test_result

    @BluetoothBaseTest.bt_test_wrap
    def test_scan_non_existent_name_filter(self):
        """Test non-existent name filter.

        Test scan filter on non-existent device name.

        Steps:
        1. Setup the scanning android device with scan filter for device name
        set to an unexpected value.
        2. Setup the advertiser android devices.
        3. Verify that no onScanResults were triggered.

        Expected Result:
        No advertisements were found.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        test_result = True
        filter_name = "{}_probably_wont_find".format(
            self.adv_ad.droid.bluetoothGetLocalName())
        self.adv_ad.droid.bleSetAdvertiseDataIncludeDeviceName(True)
        self.scn_ad.droid.bleSetScanFilterDeviceName(filter_name)
        filter_list, scan_settings, scan_callback = generate_ble_scan_objects(
            self.scn_ad.droid)
        self.scn_ad.droid.bleBuildScanFilter(filter_list)
        expected_event_name = scan_result.format(scan_callback)
        self.adv_ad.droid.bleSetAdvertiseDataIncludeDeviceName(True)
        self.adv_ad.droid.bleSetAdvertiseDataIncludeTxPowerLevel(True)
        advertise_callback, advertise_data, advertise_settings = (
            generate_ble_advertise_objects(self.adv_ad.droid))
        self.adv_ad.droid.bleStartBleAdvertising(
            advertise_callback, advertise_data, advertise_settings)
        self.scn_ad.droid.bleStartBleScan(filter_list, scan_settings,
                                          scan_callback)
        try:
            event_info = self.scn_ad.ed.pop_event(expected_event_name,
                                                  self.default_timeout)
            self.log.error("Unexpectedly found an advertiser: {}".format(
                event_info))
            test_result = False
        except Empty:
            self.log.debug("No events were found as expected.")
        self.scn_ad.droid.bleStopBleScan(scan_callback)
        self.adv_ad.droid.bleStopBleAdvertising(advertise_callback)
        return test_result

    @BluetoothBaseTest.bt_test_wrap
    def test_scan_advertisement_with_device_service_uuid_filter_expect_no_events(
            self):
        """Test scan filtering against an advertisement with no data.

        Test that exercises a service uuid filter on the scanner but no server
        uuid added to the advertisement.

        Steps:
        1. Setup the scanning android device with scan filter including a
        service uuid and mask.
        2. Setup the advertiser android devices.
        3. Verify that no onScanResults were triggered.

        Expected Result:
        Verify no advertisements found.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 1
        """
        test_result = True
        service_uuid = "00000000-0000-1000-8000-00805F9B34FB"
        service_mask = "00000000-0000-1000-8000-00805F9B34FA"
        self.adv_ad.droid.bleSetAdvertiseDataIncludeDeviceName(True)
        self.scn_ad.droid.bleSetScanFilterServiceUuid(service_uuid,
                                                      service_mask)
        self.adv_ad.droid.bleSetAdvertiseSettingsAdvertiseMode(
            AdvertiseSettingsAdvertiseMode.ADVERTISE_MODE_LOW_LATENCY.value)
        filter_list, scan_settings, scan_callback = generate_ble_scan_objects(
            self.scn_ad.droid)
        self.scn_ad.droid.bleBuildScanFilter(filter_list)
        expected_event_name = scan_result.format(scan_callback)
        self.adv_ad.droid.bleSetAdvertiseDataIncludeDeviceName(True)
        self.adv_ad.droid.bleSetAdvertiseDataIncludeTxPowerLevel(True)
        advertise_callback, advertise_data, advertise_settings = (
            generate_ble_advertise_objects(self.adv_ad.droid))
        self.adv_ad.droid.bleStartBleAdvertising(
            advertise_callback, advertise_data, advertise_settings)
        self.scn_ad.droid.bleStartBleScan(filter_list, scan_settings,
                                          scan_callback)
        worker = self.scn_ad.ed.handle_event(
            self.blescan_verify_onscanresult_event_handler,
            expected_event_name, ([1]), self.default_timeout)
        try:
            event_info = self.scn_ad.ed.pop_event(expected_event_name,
                                                  self.default_timeout)
            self.log.error("Unexpectedly found an advertiser:".format(
                event_info))
            test_result = False
        except Empty as error:
            self.log.debug("No events were found as expected.")
        self.scn_ad.droid.bleStopBleScan(scan_callback)
        self.adv_ad.droid.bleStopBleAdvertising(advertise_callback)
        return test_result

    @BluetoothBaseTest.bt_test_wrap
    def test_scan_filtering_multiple_advertisements_manufacturer_data(self):
        """Test scan filtering against multiple varying advertisements.

        Test scan filtering against multiple varying advertisements. The first
        advertisement will have partial manufacturer data that matches the
        the full manufacturer data in the second advertisement.

        Steps:
        1. Setup up an advertisement with manufacturer data "1,2,3".
        2. Setup a second advertisement with manufacturer data
        "1,2,3,4,5,6,7,8".
        3. Start advertising on each advertisement.
        4. Create a scan filter that includes manufacturer data "1,2,3".

        Expected Result:
        TBD. Right now Shamu finds only the first advertisement with
        manufacturer data "1,2,3".

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        test_result = True
        self.adv_ad.droid.bleSetAdvertiseSettingsAdvertiseMode(
            AdvertiseSettingsAdvertiseMode.ADVERTISE_MODE_LOW_LATENCY.value)
        self.adv_ad.droid.bleAddAdvertiseDataManufacturerId(117, "1,2,3")
        advertise_callback, advertise_data, advertise_settings = (
            generate_ble_advertise_objects(self.adv_ad.droid))
        self.adv_ad.droid.bleSetAdvertiseSettingsAdvertiseMode(
            AdvertiseSettingsAdvertiseMode.ADVERTISE_MODE_LOW_LATENCY.value)
        self.adv_ad.droid.bleAddAdvertiseDataManufacturerId(117,
                                                            "1,2,3,4,5,6,7,8")
        advertise_callback1, advertise_data1, advertise_settings1 = (
            generate_ble_advertise_objects(self.adv_ad.droid))
        self.adv_ad.droid.bleStartBleAdvertising(
            advertise_callback, advertise_data, advertise_settings)
        self.adv_ad.droid.bleStartBleAdvertising(
            advertise_callback1, advertise_data1, advertise_settings1)

        filter_list = self.scn_ad.droid.bleGenFilterList()
        self.scn_ad.droid.bleSetScanSettingsScanMode(
            ScanSettingsScanMode.SCAN_MODE_LOW_LATENCY.value)
        scan_settings = self.scn_ad.droid.bleBuildScanSetting()
        scan_callback = self.scn_ad.droid.bleGenScanCallback()
        self.scn_ad.droid.bleSetScanFilterManufacturerData(117, "1,2,3",
                                                           "127,127,127")
        self.scn_ad.droid.bleBuildScanFilter(filter_list)
        self.scn_ad.droid.bleStartBleScan(filter_list, scan_settings,
                                          scan_callback)
        return test_result

    @BluetoothBaseTest.bt_test_wrap
    def test_scan_filter_device_address(self):
        """Test scan filtering of a device address.

        This test will have to create two scanning instances. The first will
        have no filters and will find the generic advertisement's mac address.
        The second will have a filter of the found mac address.

        Steps:
        1. Start a generic advertisement.
        2. Start a generic scanner.
        3. Find the advertisement and extract the mac address.
        4. Stop the first scanner.
        5. Create a new scanner with scan filter with a mac address filter of
        what was found in step 3.
        6. Start the scanner.

        Expected Result:
        Verify that the advertisement was found in the second scan instance.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 1
        """
        test_result = True
        self.scn_ad.droid.bleSetScanSettingsScanMode(
            ScanSettingsScanMode.SCAN_MODE_LOW_LATENCY.value)
        filter_list, scan_settings, scan_callback = generate_ble_scan_objects(
            self.scn_ad.droid)
        expected_event_name = scan_result.format(scan_callback)
        self.adv_ad.droid.bleSetAdvertiseSettingsAdvertiseMode(
            AdvertiseSettingsAdvertiseMode.ADVERTISE_MODE_LOW_LATENCY.value)
        advertise_callback, advertise_data, advertise_settings = (
            generate_ble_advertise_objects(self.adv_ad.droid))
        self.adv_ad.droid.bleStartBleAdvertising(
            advertise_callback, advertise_data, advertise_settings)
        self.scn_ad.droid.bleStartBleScan(filter_list, scan_settings,
                                          scan_callback)
        event_info = self.scn_ad.ed.pop_event(expected_event_name,
                                              self.default_timeout)
        mac_address = event_info['data']['Result']['deviceInfo']['address']
        self.log.info("Filter advertisement with address {}".format(
            mac_address))
        self.scn_ad.droid.bleStopBleScan(scan_callback)
        self.scn_ad.droid.bleSetScanSettingsScanMode(
            ScanSettingsScanMode.SCAN_MODE_LOW_LATENCY.value)
        self.scn_ad.droid.bleSetScanFilterDeviceAddress(mac_address)
        filter_list2, scan_settings2, scan_callback2 = (
            generate_ble_scan_objects(self.scn_ad.droid))

        self.scn_ad.droid.bleBuildScanFilter(filter_list2)
        self.scn_ad.droid.bleStartBleScan(filter_list2, scan_settings2,
                                          scan_callback2)
        expected_event_name = scan_result.format(scan_callback2)
        found_event = self.scn_ad.ed.pop_event(expected_event_name,
                                               self.default_timeout)
        if (found_event['data']['Result']['deviceInfo']['address'] !=
                mac_address):
            test_result = False
        self.scn_ad.droid.bleStopBleScan(scan_callback2)
        self.adv_ad.droid.bleStopBleAdvertising(advertise_callback)
        return test_result
