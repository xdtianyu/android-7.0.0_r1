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
This test script exercises different opportunistic scan scenarios.
It is expected that the second AndroidDevice is able to advertise.

This test script was designed with this setup in mind:
Shield box one: Android Device, Android Device
"""

from queue import Empty

from acts.test_utils.bt.BluetoothBaseTest import BluetoothBaseTest
from acts.test_utils.bt.BleEnum import ScanSettingsScanMode
from acts.test_utils.bt.BleEnum import ScanSettingsScanMode
from acts.test_utils.bt.bt_test_utils import batch_scan_result
from acts.test_utils.bt.bt_test_utils import cleanup_scanners_and_advertisers
from acts.test_utils.bt.bt_test_utils import generate_ble_advertise_objects
from acts.test_utils.bt.bt_test_utils import generate_ble_scan_objects
from acts.test_utils.bt.bt_test_utils import get_advanced_droid_list
from acts.test_utils.bt.bt_test_utils import reset_bluetooth
from acts.test_utils.bt.bt_test_utils import scan_result


class BleOpportunisticScanTest(BluetoothBaseTest):
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
        self.tests = (
            "test_scan_result_no_advertisement",
            "test_scan_result_no_advertisement",
            "test_scan_result",
            "test_batch_scan_result_not_expected",
            "test_scan_result_not_expected",
            "test_max_opportunistic_scan_instances",
            "test_discover_opportunistic_scan_result_off_secondary_scan_filter",
            "test_negative_opportunistic_scan_filter_result_off_secondary_scan_result",
            "test_opportunistic_scan_filter_result_off_secondary_scan_result",
        )
        if self.droid_list[0]['batch_scan_supported']:
            self.tests = self.tests + (
                "test_batch_scan_result",
                "test_max_opportunistic_batch_scan_instances", )

    def teardown_test(self):
        cleanup_scanners_and_advertisers(
            self.scn_ad, self.active_adv_callback_list, self.adv_ad,
            self.active_adv_callback_list)
        self.active_adv_callback_list = []
        self.active_scan_callback_list = []

    def on_exception(self, test_name, begin_time):
        reset_bluetooth(self.android_devices)

    def _setup_generic_advertisement(self):
        adv_callback, adv_data, adv_settings = generate_ble_advertise_objects(
            self.adv_ad.droid)
        self.adv_ad.droid.bleStartBleAdvertising(adv_callback, adv_data,
                                                 adv_settings)
        self.active_adv_callback_list.append(adv_callback)

    def _verify_no_events_found(self, event_name):
        try:
            event = self.scn_ad.ed.pop_event(event_name, self.default_timeout)
            self.log.error("Found an event when none was expected: {}".format(
                event))
            return False
        except Empty:
            self.log.info("No scan result found as expected.")
            return True

    @BluetoothBaseTest.bt_test_wrap
    def test_scan_result_no_advertisement(self):
        """Test opportunistic scan with no advertisement.

        Tests opportunistic scan where there are no advertisements. This should
        not find any onScanResults.

        Steps:
        1. Initialize scanner with scan mode set to opportunistic mode.
        2. Start scanning on dut 0
        3. Pop onScanResults event on the scanner

        Expected Result:
        Find no advertisements with the opportunistic scan instance.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Scanning, Opportunistic Scan
        Priority: 1
        """
        self.scn_ad.droid.bleSetScanSettingsScanMode(
            ScanSettingsScanMode.SCAN_MODE_OPPORTUNISTIC.value)
        filter_list, scan_settings, scan_callback = generate_ble_scan_objects(
            self.scn_ad.droid)
        self.scn_ad.droid.bleStartBleScan(filter_list, scan_settings,
                                          scan_callback)
        self.active_scan_callback_list.append(scan_callback)
        if not self._verify_no_events_found(scan_result.format(scan_callback)):
            return False
        self.scn_ad.droid.bleStopBleScan(scan_callback)
        return True

    @BluetoothBaseTest.bt_test_wrap
    def test_batch_scan_result_no_advertisement(self):
        """Test batch opportunistic scan without an advertisement.

        Tests opportunistic scan where there are no advertisements. This should
        not find any onBatchScanResult.

        Steps:
        1. Initialize scanner with scan mode set to opportunistic mode.
        2. Set report delay seconds such that onBatchScanResult events are
        expected
        2. Start scanning on dut 0
        3. Pop onBatchScanResult event on the scanner

        Expected Result:
        Find no advertisements with the opportunistic scan instance.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Scanning, Opportunistic Scan, Batch Scanning
        Priority: 1
        """
        self.scn_ad.droid.bleSetScanSettingsScanMode(
            ScanSettingsScanMode.SCAN_MODE_OPPORTUNISTIC.value)
        self.scn_ad.droid.bleSetScanSettingsReportDelayMillis(
            self.report_delay)
        filter_list, scan_settings, scan_callback = generate_ble_scan_objects(
            self.scn_ad.droid)
        self.scn_ad.droid.bleStartBleScan(filter_list, scan_settings,
                                          scan_callback)
        self.active_scan_callback_list.append(scan_callback)
        if not self._verify_no_events_found(batch_scan_result.format(
                scan_callback)):
            return False
        self.scn_ad.droid.bleStopBleScan(scan_callback)
        return True

    @BluetoothBaseTest.bt_test_wrap
    def test_scan_result(self):
        """Test opportunistic scan with an advertisement.

        Tests opportunistic scan where it will only report scan results when
        other registered scanners find results.

        Steps:
        1. Initialize advertiser and start advertisement on dut1
        2. Initialize scanner with scan mode set to opportunistic mode on dut0
        and start scanning
        3. Try to find an event, expect none.
        4. Start a second scanner on dut0, with any other mode set
        5. Pop onScanResults event on the second scanner
        6. Pop onScanResults event on the first scanner

        Expected Result:
        Scan result is found on the opportunistic scan instance.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Scanning, Opportunistic Scan
        Priority: 1
        """
        self._setup_generic_advertisement()
        self.scn_ad.droid.bleSetScanSettingsScanMode(
            ScanSettingsScanMode.SCAN_MODE_OPPORTUNISTIC.value)
        filter_list, scan_settings, scan_callback = generate_ble_scan_objects(
            self.scn_ad.droid)

        self.scn_ad.droid.bleStartBleScan(filter_list, scan_settings,
                                          scan_callback)
        self.active_scan_callback_list.append(scan_callback)
        if not self._verify_no_events_found(scan_result.format(scan_callback)):
            return False
        self.scn_ad.droid.bleSetScanSettingsScanMode(
            ScanSettingsScanMode.SCAN_MODE_LOW_LATENCY.value)
        filter_list2, scan_settings2, scan_callback2 = (
            generate_ble_scan_objects(self.scn_ad.droid))
        self.scn_ad.droid.bleStartBleScan(filter_list2, scan_settings2,
                                          scan_callback2)
        self.active_scan_callback_list.append(scan_callback2)
        self.scn_ad.ed.pop_event(
            scan_result.format(scan_callback2), self.default_timeout)
        self.scn_ad.ed.pop_event(
            scan_result.format(scan_callback), self.default_timeout)
        return True

    @BluetoothBaseTest.bt_test_wrap
    def test_batch_scan_result(self):
        """Test batch opportunistic scan with advertisement.

        Tests opportunistic scan where it will only report scan results when
        other registered scanners find results. Set the report delay millis such
        that an onBatchScanResult is expected.

        Steps:
        1. Initialize advertiser and start advertisement on dut1
        2. Initialize scanner with scan mode set to opportunistic mode and
        set scan settings report delay seconds such that a batch scan is
        expected
        3. Start scanning on dut 0
        4. Try to find an event, expect none.
        5. Start a second scanner on dut0, with any other mode set and set scan
        settings report delay millis such that an onBatchScanResult is expected
        6. Pop onBatchScanResult event on the second scanner
        7. Pop onBatchScanResult event on the first scanner

        Expected Result:
        Find a batch scan result on both opportunistic scan instances.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Scanning, Opportunistic Scan, Batch Scanning
        Priority: 1
        """
        self._setup_generic_advertisement()
        self.scn_ad.droid.bleSetScanSettingsScanMode(
            ScanSettingsScanMode.SCAN_MODE_OPPORTUNISTIC.value)
        self.scn_ad.droid.bleSetScanSettingsReportDelayMillis(
            self.report_delay)
        filter_list, scan_settings, scan_callback = generate_ble_scan_objects(
            self.scn_ad.droid)
        self.scn_ad.droid.bleStartBleScan(filter_list, scan_settings,
                                          scan_callback)
        self.active_scan_callback_list.append(scan_callback)
        if not self._verify_no_events_found(batch_scan_result.format(
                scan_callback)):
            return False
        self.scn_ad.droid.bleSetScanSettingsReportDelayMillis(
            self.report_delay)
        self.scn_ad.droid.bleSetScanSettingsScanMode(
            ScanSettingsScanMode.SCAN_MODE_LOW_LATENCY.value)
        filter_list2 = self.scn_ad.droid.bleGenFilterList()
        scan_settings2 = self.scn_ad.droid.bleBuildScanSetting()
        scan_callback2 = self.scn_ad.droid.bleGenScanCallback()
        self.scn_ad.droid.bleStartBleScan(filter_list2, scan_settings2,
                                          scan_callback2)
        self.active_scan_callback_list.append(scan_callback2)
        self.scn_ad.ed.pop_event(
            batch_scan_result.format(scan_callback2), self.default_timeout)
        self.scn_ad.ed.pop_event(
            batch_scan_result.format(scan_callback), self.default_timeout)
        return True

    @BluetoothBaseTest.bt_test_wrap
    def test_batch_scan_result_not_expected(self):
        """Test opportunistic batch scan without expecting an event.

        Tests opportunistic scan where it will only report scan results when
        other registered scanners find results. Set the report delay millis such
        that a batch scan is not expected.

        Steps:
        1. Initialize advertiser and start advertisement on dut1
        2. Initialize scanner with scan mode set to opportunistic mode and
        set scan settings report delay seconds such that a batch scan is
        expected.
        3. Start scanning on dut 0
        4. Try to find an event, expect none.
        5. Start a second scanner on dut0, with any other mode set and set scan
        settings report delay millis to 0 such that an onBatchScanResult is not
        expected.
        6. Pop onScanResults event on the second scanner
        7. Pop onBatchScanResult event on the first scanner

        Expected Result:
        Batch scan result is not expected on opportunistic scan instance.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Scanning, Opportunistic Scan, Batch Scanning
        Priority: 1
        """
        self._setup_generic_advertisement()
        self.scn_ad.droid.bleSetScanSettingsScanMode(
            ScanSettingsScanMode.SCAN_MODE_OPPORTUNISTIC.value)
        self.scn_ad.droid.bleSetScanSettingsReportDelayMillis(
            self.report_delay)
        filter_list, scan_settings, scan_callback = generate_ble_scan_objects(
            self.scn_ad.droid)
        self.scn_ad.droid.bleStartBleScan(filter_list, scan_settings,
                                          scan_callback)
        self.active_scan_callback_list.append(scan_callback)
        if not self._verify_no_events_found(batch_scan_result.format(
                scan_callback)):

            return False
        self.scn_ad.droid.bleSetScanSettingsScanMode(
            ScanSettingsScanMode.SCAN_MODE_LOW_LATENCY.value)
        filter_list2, scan_settings2, scan_callback2 = (
            generate_ble_scan_objects(self.scn_ad.droid))
        self.scn_ad.droid.bleStartBleScan(filter_list2, scan_settings2,
                                          scan_callback2)
        self.active_scan_callback_list.append(scan_callback2)
        self.scn_ad.ed.pop_event(
            scan_result.format(scan_callback2), self.default_timeout)
        return self._verify_no_events_found(batch_scan_result.format(
            scan_callback))

    @BluetoothBaseTest.bt_test_wrap
    def test_scan_result_not_expected(self):
        """Test opportunistic scan without expecting an event.

        Tests opportunistic scan where it will only report batch scan results
        when other registered scanners find results.

        Steps:
        1. Initialize advertiser and start advertisement on dut1
        2. Initialize scanner with scan mode set to opportunistic mode.
        3. Start scanning on dut 0
        4. Try to find an event, expect none.
        5. Start a second scanner on dut0, with any other mode set and set scan
        settings
        report delay millis such that an onBatchScanResult is expected
        6. Pop onBatchScanResult event on the second scanner
        7. Pop onScanResults event on the first scanner

        Expected Result:
        Scan result is not expected on opportunistic scan instance.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Scanning, Opportunistic Scan
        Priority: 1
        """
        self._setup_generic_advertisement()
        self.scn_ad.droid.bleSetScanSettingsScanMode(
            ScanSettingsScanMode.SCAN_MODE_OPPORTUNISTIC.value)
        filter_list = self.scn_ad.droid.bleGenFilterList()
        scan_settings = self.scn_ad.droid.bleBuildScanSetting()
        scan_callback = self.scn_ad.droid.bleGenScanCallback()
        self.scn_ad.droid.bleStartBleScan(filter_list, scan_settings,
                                          scan_callback)
        self.active_scan_callback_list.append(scan_callback)
        if not self._verify_no_events_found(scan_result.format(scan_callback)):
            return False
        self.scn_ad.droid.bleSetScanSettingsReportDelayMillis(
            self.report_delay)
        self.scn_ad.droid.bleSetScanSettingsScanMode(
            ScanSettingsScanMode.SCAN_MODE_LOW_LATENCY.value)
        filter_list2, scan_settings2, scan_callback2 = (
            generate_ble_scan_objects(self.scn_ad.droid))
        self.scn_ad.droid.bleStartBleScan(filter_list2, scan_settings2,
                                          scan_callback2)
        self.active_scan_callback_list.append(scan_callback2)
        self.scn_ad.ed.pop_event(
            batch_scan_result.format(scan_callback2), self.default_timeout)
        return self._verify_no_events_found(scan_result.format(scan_callback))

    @BluetoothBaseTest.bt_test_wrap
    def test_max_opportunistic_scan_instances(self):
        """Test max number of opportunistic scan instances.

        Tests max instances of opportunistic scans. Each instances should
        find an onScanResults event.

        Steps:
        1. Initialize advertiser and start advertisement on dut1
        2. Set scan settings to opportunistic scan on dut0 scan instance
        3. Start scan scan from step 2
        4. Repeat step two and three until there are max_scan_instances-1 scan
        instances
        5. Start a regular ble scan on dut0 with the last available scan
        instance
        6. Pop onScanResults event on all scan instances

        Expected Result:
        Each opportunistic scan instance finds a advertisement.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Scanning, Opportunistic Scan
        Priority: 1
        """
        self._setup_generic_advertisement()
        for _ in range(self.max_scan_instances - 1):
            self.scn_ad.droid.bleSetScanSettingsScanMode(
                ScanSettingsScanMode.SCAN_MODE_OPPORTUNISTIC.value)
            filter_list = self.scn_ad.droid.bleGenFilterList()
            scan_settings = self.scn_ad.droid.bleBuildScanSetting()
            scan_callback = self.scn_ad.droid.bleGenScanCallback()
            self.scn_ad.droid.bleStartBleScan(filter_list, scan_settings,
                                              scan_callback)
            self.active_scan_callback_list.append(scan_callback)

        self.scn_ad.droid.bleSetScanSettingsScanMode(
            ScanSettingsScanMode.SCAN_MODE_LOW_LATENCY.value)
        filter_list2, scan_settings2, scan_callback2 = (
            generate_ble_scan_objects(self.scn_ad.droid))
        self.scn_ad.droid.bleStartBleScan(filter_list2, scan_settings2,
                                          scan_callback2)
        self.active_scan_callback_list.append(scan_callback2)

        for callback in self.active_scan_callback_list:
            self.scn_ad.ed.pop_event(
                scan_result.format(callback), self.default_timeout)

        return True

    @BluetoothBaseTest.bt_test_wrap
    def test_max_opportunistic_batch_scan_instances(self):
        """Test max opportunistic batch scan instances.

        Tests max instances of opportunistic batch scans. Each instances should
        find an onBatchScanResult event.

        Steps:
        1. Initialize advertiser and start advertisement on dut1
        2. Set scan settings to opportunistic scan on dut0 scan instance and
        set report delay seconds such that an onBatchScanResult is expected
        3. Start scan scan from step 2
        4. Repeat step two and three until there are max_scan_instances-1 scan
        instances
        5. Start a regular ble scan on dut0 with the last available scan
        instance
        6. Pop onBatchScanResult event on all scan instances

        Expected Result:
        Each opportunistic scan instance finds an advertisement.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Scanning, Opportunistic Scan, Batch Scanning
        Priority: 1
        """
        self._setup_generic_advertisement()
        for _ in range(self.max_scan_instances - 1):
            self.scn_ad.droid.bleSetScanSettingsScanMode(
                ScanSettingsScanMode.SCAN_MODE_OPPORTUNISTIC.value)
            self.scn_ad.droid.bleSetScanSettingsReportDelayMillis(
                self.report_delay)
            filter_list = self.scn_ad.droid.bleGenFilterList()
            scan_settings = self.scn_ad.droid.bleBuildScanSetting()
            scan_callback = self.scn_ad.droid.bleGenScanCallback()
            self.scn_ad.droid.bleStartBleScan(filter_list, scan_settings,
                                              scan_callback)
            self.active_scan_callback_list.append(scan_callback)

        self.scn_ad.droid.bleSetScanSettingsScanMode(
            ScanSettingsScanMode.SCAN_MODE_LOW_LATENCY.value)
        self.scn_ad.droid.bleSetScanSettingsReportDelayMillis(
            self.report_delay)
        filter_list2, scan_settings2, scan_callback2 = (
            generate_ble_scan_objects(self.scn_ad.droid))
        self.scn_ad.droid.bleStartBleScan(filter_list2, scan_settings2,
                                          scan_callback2)
        self.active_scan_callback_list.append(scan_callback2)

        for callback in self.active_scan_callback_list:
            self.scn_ad.ed.pop_event(
                batch_scan_result.format(callback), self.default_timeout)

        return True

    @BluetoothBaseTest.bt_test_wrap
    def test_discover_opportunistic_scan_result_off_secondary_scan_filter(
            self):
        """Test opportunistic scan result from secondary scan filter.

        Tests opportunistic scan where the secondary scan instance does not find
        an advertisement but the scan instance with scan mode set to
        opportunistic scan will find an advertisement.

        Steps:
        1. Initialize advertiser and start advertisement on dut1 (make sure the
        advertisement is not advertising the device name)
        2. Set scan settings to opportunistic scan on dut0 scan instance
        3. Start scan scan from step 2
        4. Try to find an event, expect none
        5. Start a second scanner on dut0, with any other mode set and set the
        scan filter device name to "opp_test"
        6. Pop onScanResults from the second scanner
        7. Expect no events
        8. Pop onScanResults from the first scanner

        Expected Result:
        Opportunistic scan instance finds an advertisement.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Scanning, Opportunistic Scan
        Priority: 1
        """
        self._setup_generic_advertisement()
        self.scn_ad.droid.bleSetScanSettingsScanMode(
            ScanSettingsScanMode.SCAN_MODE_OPPORTUNISTIC.value)
        filter_list, scan_settings, scan_callback = generate_ble_scan_objects(
            self.scn_ad.droid)

        self.scn_ad.droid.bleStartBleScan(filter_list, scan_settings,
                                          scan_callback)
        self.active_scan_callback_list.append(scan_callback)
        if not self._verify_no_events_found(scan_result.format(scan_callback)):
            return False
        self.scn_ad.droid.bleSetScanSettingsScanMode(
            ScanSettingsScanMode.SCAN_MODE_LOW_LATENCY.value)
        filter_list2, scan_settings2, scan_callback2 = (
            generate_ble_scan_objects(self.scn_ad.droid))
        self.scn_ad.droid.bleSetScanFilterDeviceName("opp_test")
        self.scn_ad.droid.bleBuildScanFilter(filter_list2)
        self.scn_ad.droid.bleStartBleScan(filter_list2, scan_settings2,
                                          scan_callback2)
        self.active_scan_callback_list.append(scan_callback2)
        if not self._verify_no_events_found(scan_result.format(
                scan_callback2)):
            return False
        self.scn_ad.ed.pop_event(
            scan_result.format(scan_callback), self.default_timeout)
        return True

    @BluetoothBaseTest.bt_test_wrap
    def test_negative_opportunistic_scan_filter_result_off_secondary_scan_result(
            self):
        """Test opportunistic scan not found scenario.

        Tests opportunistic scan where the secondary scan instance does find an
        advertisement but the scan instance with scan mode set to opportunistic
        scan does not find an advertisement due to mismatched scan filters.

        Steps:
        1. Initialize advertiser and start advertisement on dut1 (make sure the
        advertisement is not advertising the device name)
        2. Set scan settings to opportunistic scan on dut0 scan instance and set
        the scan filter device name to "opp_test"
        3. Start scan scan from step 2
        4. Try to find an event, expect none
        5. Start a second scanner on dut0, with any other mode set
        6. Pop onScanResults from the second scanner
        7. Pop onScanResults from the first scanner
        8. Expect no events

        Expected Result:
        Opportunistic scan instance doesn't find any advertisements.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Scanning, Opportunistic Scan
        Priority: 1
        """
        self._setup_generic_advertisement()
        self.scn_ad.droid.bleSetScanSettingsScanMode(
            ScanSettingsScanMode.SCAN_MODE_OPPORTUNISTIC.value)
        filter_list, scan_settings, scan_callback = generate_ble_scan_objects(
            self.scn_ad.droid)
        self.scn_ad.droid.bleSetScanFilterDeviceName("opp_test")
        self.scn_ad.droid.bleBuildScanFilter(filter_list)
        self.scn_ad.droid.bleStartBleScan(filter_list, scan_settings,
                                          scan_callback)
        self.active_scan_callback_list.append(scan_callback)
        if not self._verify_no_events_found(scan_result.format(scan_callback)):
            return False
        self.scn_ad.droid.bleSetScanSettingsScanMode(
            ScanSettingsScanMode.SCAN_MODE_LOW_LATENCY.value)
        filter_list2, scan_settings2, scan_callback2 = (
            generate_ble_scan_objects(self.scn_ad.droid))
        self.scn_ad.droid.bleStartBleScan(filter_list2, scan_settings2,
                                          scan_callback2)
        self.active_scan_callback_list.append(scan_callback2)
        self.scn_ad.ed.pop_event(
            scan_result.format(scan_callback2), self.default_timeout)
        return self._verify_no_events_found(scan_result.format(scan_callback))

    @BluetoothBaseTest.bt_test_wrap
    def test_opportunistic_scan_filter_result_off_secondary_scan_result(self):
        """Test opportunistic scan from a secondary scan result.

        Tests opportunistic scan where the scan filters are the same between the
        first scan instance with opportunistic scan set and the second instance
        with any other mode set.

        Steps:
        1. Initialize advertiser and start advertisement on dut1 (make sure the
        advertisement is not advertising the device name)
        2. Set scan settings to opportunistic scan on dut0 scan instance and set
        the scan filter device name to the advertiser's device name
        3. Start scan scan from step 2
        4. Try to find an event, expect none
        5. Start a second scanner on dut0, with any other mode set and set the
        scan filter device name to the advertiser's device name
        6. Pop onScanResults from the second scanner
        7. Pop onScanResults from the first scanner

        Expected Result:
        Opportunistic scan instance finds a advertisement.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Scanning, Opportunistic Scan
        Priority: 1
        """
        self.adv_ad.droid.bleSetAdvertiseDataIncludeDeviceName(True)
        self._setup_generic_advertisement()
        adv_device_name = self.adv_ad.droid.bluetoothGetLocalName()
        self.scn_ad.droid.bleSetScanSettingsScanMode(
            ScanSettingsScanMode.SCAN_MODE_OPPORTUNISTIC.value)
        filter_list, scan_settings, scan_callback = generate_ble_scan_objects(
            self.scn_ad.droid)
        self.scn_ad.droid.bleSetScanFilterDeviceName(adv_device_name)
        self.scn_ad.droid.bleBuildScanFilter(filter_list)
        self.scn_ad.droid.bleStartBleScan(filter_list, scan_settings,
                                          scan_callback)
        self.active_scan_callback_list.append(scan_callback)
        if not self._verify_no_events_found(scan_result.format(scan_callback)):
            return False
        self.scn_ad.droid.bleSetScanSettingsScanMode(
            ScanSettingsScanMode.SCAN_MODE_LOW_LATENCY.value)
        filter_list2, scan_settings2, scan_callback2 = (
            generate_ble_scan_objects(self.scn_ad.droid))
        self.scn_ad.droid.bleSetScanFilterDeviceName(adv_device_name)
        self.scn_ad.droid.bleBuildScanFilter(filter_list2)
        self.scn_ad.droid.bleStartBleScan(filter_list2, scan_settings2,
                                          scan_callback2)
        self.active_scan_callback_list.append(scan_callback2)
        self.scn_ad.ed.pop_event(
            scan_result.format(scan_callback2), self.default_timeout)
        self.scn_ad.ed.pop_event(
            scan_result.format(scan_callback), self.default_timeout)
        return True
