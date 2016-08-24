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
Test script to exercise Ble Scan Api's. This exercises all getters and
setters. This is important since there is a builder object that is immutable
after you set all attributes of each object. If this test suite doesn't pass,
then other test suites utilising Ble Scanner will also fail.
"""

from acts.controllers.android import SL4AAPIError
from acts.test_utils.bt.BluetoothBaseTest import BluetoothBaseTest
from acts.test_utils.bt.BleEnum import ScanSettingsCallbackType
from acts.test_utils.bt.BleEnum import ScanSettingsScanMode
from acts.test_utils.bt.BleEnum import ScanSettingsScanResultType
from acts.test_utils.bt.BleEnum import ScanSettingsReportDelaySeconds
from acts.test_utils.bt.BleEnum import Uuids


class BleScanResultsError(Exception):
    """Error in getting scan results"""


class BleScanVerificationError(Exception):
    """Error in comparing BleScan results"""


class BleSetScanSettingsError(Exception):
    """Error in setting Ble Scan Settings"""


class BleSetScanFilterError(Exception):
    """Error in setting Ble Scan Settings"""


class BleScanApiTest(BluetoothBaseTest):

    def __init__(self, controllers):
        BluetoothBaseTest.__init__(self, controllers)
        self.droid = self.android_devices[0].droid

    def _format_defaults(self, input):
        """
        Creates a dictionary of default ScanSetting and ScanFilter Values.
        :return: input: dict
        """
        if 'ScanSettings' not in input.keys():
            input['ScanSettings'] = (
                ScanSettingsCallbackType.CALLBACK_TYPE_ALL_MATCHES.value, 0,
                ScanSettingsScanMode.SCAN_MODE_LOW_POWER.value,
                ScanSettingsScanResultType.SCAN_RESULT_TYPE_FULL.value)
        if 'ScanFilterManufacturerDataId' not in input.keys():
            input['ScanFilterManufacturerDataId'] = -1
        if 'ScanFilterDeviceName' not in input.keys():
            input['ScanFilterDeviceName'] = None
        if 'ScanFilterDeviceAddress' not in input.keys():
            input['ScanFilterDeviceAddress'] = None
        if 'ScanFilterManufacturerData' not in input.keys():
            input['ScanFilterManufacturerData'] = ""
        return input

    def validate_scan_settings_helper(self, input, droid):
        """
        Validates each input of the scan settings object that is matches what
        was set or not set such that it matches the defaults.
        :return: False at any point something doesn't match. True if everything
        matches.
        """
        filter_list = droid.bleGenFilterList()
        if 'ScanSettings' in input.keys():
            try:
                droid.bleSetScanSettingsCallbackType(input['ScanSettings'][0])
                droid.bleSetScanSettingsReportDelayMillis(input[
                    'ScanSettings'][1])
                droid.bleSetScanSettingsScanMode(input['ScanSettings'][2])
                droid.bleSetScanSettingsResultType(input['ScanSettings'][3])
            except SL4AAPIError as error:
                self.log.debug("Set Scan Settings failed with: ".format(error))
                return False
        if 'ScanFilterDeviceName' in input.keys():
            try:
                droid.bleSetScanFilterDeviceName(input['ScanFilterDeviceName'])
            except SL4AAPIError as error:
                self.log.debug("Set Scan Filter Device Name failed with: {}"
                               .format(error))
                return False
        if 'ScanFilterDeviceAddress' in input.keys():
            try:
                droid.bleSetScanFilterDeviceAddress(input[
                    'ScanFilterDeviceAddress'])
            except SL4AAPIError as error:
                self.log.debug("Set Scan Filter Device Address failed with: {}"
                               .format(error))
                return False
        if ('ScanFilterManufacturerDataId' in input.keys() and
                'ScanFilterManufacturerDataMask' in input.keys()):
            try:
                droid.bleSetScanFilterManufacturerData(
                    input['ScanFilterManufacturerDataId'],
                    input['ScanFilterManufacturerData'],
                    input['ScanFilterManufacturerDataMask'])
            except SL4AAPIError as error:
                self.log.debug("Set Scan Filter Manufacturer info with data "
                               "mask failed with: {}".format(error))
                return False
        if ('ScanFilterManufacturerDataId' in input.keys() and
                'ScanFilterManufacturerData' in input.keys() and
                'ScanFilterManufacturerDataMask' not in input.keys()):
            try:
                droid.bleSetScanFilterManufacturerData(
                    input['ScanFilterManufacturerDataId'],
                    input['ScanFilterManufacturerData'])
            except SL4AAPIError as error:
                self.log.debug(
                    "Set Scan Filter Manufacturer info failed with: "
                    "{}".format(error))
                return False
        if ('ScanFilterServiceUuid' in input.keys() and
                'ScanFilterServiceMask' in input.keys()):

            droid.bleSetScanFilterServiceUuid(input['ScanFilterServiceUuid'],
                                              input['ScanFilterServiceMask'])

        input = self._format_defaults(input)
        scan_settings_index = droid.bleBuildScanSetting()
        scan_settings = (
            droid.bleGetScanSettingsCallbackType(scan_settings_index),
            droid.bleGetScanSettingsReportDelayMillis(scan_settings_index),
            droid.bleGetScanSettingsScanMode(scan_settings_index),
            droid.bleGetScanSettingsScanResultType(scan_settings_index))

        scan_filter_index = droid.bleBuildScanFilter(filter_list)
        device_name_filter = droid.bleGetScanFilterDeviceName(
            filter_list, scan_filter_index)
        device_address_filter = droid.bleGetScanFilterDeviceAddress(
            filter_list, scan_filter_index)
        manufacturer_id = droid.bleGetScanFilterManufacturerId(
            filter_list, scan_filter_index)
        manufacturer_data = droid.bleGetScanFilterManufacturerData(
            filter_list, scan_filter_index)

        if scan_settings != input['ScanSettings']:
            self.log.debug("Scan Settings did not match. expected: {}, found: "
                           "{}".format(input['ScanSettings'], scan_settings))
            return False
        if device_name_filter != input['ScanFilterDeviceName']:
            self.log.debug("Scan Filter device name did not match. expected: "
                           "{}, found {}".format(input['ScanFilterDeviceName'],
                                                 device_name_filter))
            return False
        if device_address_filter != input['ScanFilterDeviceAddress']:
            self.log.debug("Scan Filter address name did not match. expected: "
                           "{}, found: {}".format(input[
                               'ScanFilterDeviceAddress'],
                                                  device_address_filter))
            return False
        if manufacturer_id != input['ScanFilterManufacturerDataId']:
            self.log.debug("Scan Filter manufacturer data id did not match. "
                           "expected: {}, found: {}".format(input[
                               'ScanFilterManufacturerDataId'],
                                                            manufacturer_id))
            return False
        if manufacturer_data != input['ScanFilterManufacturerData']:
            self.log.debug("Scan Filter manufacturer data did not match. "
                           "expected: {}, found: {}".format(input[
                               'ScanFilterManufacturerData'],
                                                            manufacturer_data))
            return False
        if 'ScanFilterManufacturerDataMask' in input.keys():
            manufacturer_data_mask = droid.bleGetScanFilterManufacturerDataMask(
                filter_list, scan_filter_index)
            if manufacturer_data_mask != input[
                    'ScanFilterManufacturerDataMask']:
                self.log.debug(
                    "Manufacturer data mask did not match. expected:"
                    " {}, found: {}".format(input[
                        'ScanFilterManufacturerDataMask'],
                                            manufacturer_data_mask))

                return False
        if ('ScanFilterServiceUuid' in input.keys() and
                'ScanFilterServiceMask' in input.keys()):

            expected_service_uuid = input['ScanFilterServiceUuid']
            expected_service_mask = input['ScanFilterServiceMask']
            service_uuid = droid.bleGetScanFilterServiceUuid(filter_list,
                                                             scan_filter_index)
            service_mask = droid.bleGetScanFilterServiceUuidMask(
                filter_list, scan_filter_index)
            if service_uuid != expected_service_uuid.lower():
                self.log.debug("Service uuid did not match. expected: {}, "
                               "found {}".format(expected_service_uuid,
                                                 service_uuid))
                return False
            if service_mask != expected_service_mask.lower():
                self.log.debug("Service mask did not match. expected: {}, "
                               "found {}".format(expected_service_mask,
                                                 service_mask))
                return False
        self.scan_settings_index = scan_settings_index
        self.filter_list = filter_list
        self.scan_callback = droid.bleGenScanCallback()
        return True

    @BluetoothBaseTest.bt_test_wrap
    def test_start_ble_scan_with_default_settings(self):
        """Test LE scan with default settings.

        Test to validate all default scan settings values.

        Steps:
        1. Create LE scan objects.
        2. Start LE scan.

        Expected Result:
        Scan starts successfully and matches expected settings.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Scanning
        Priority: 1
        """
        input = {}
        return self.validate_scan_settings_helper(input, self.droid)

    @BluetoothBaseTest.bt_test_wrap
    def test_stop_ble_scan_default_settings(self):
        """Test stopping an LE scan.

        Test default scan settings on an actual scan. Verify it can also stop
        the scan.

        Steps:
        1. Validate default scan settings.
        2. Start ble scan.
        3. Stop ble scan.

        Expected Result:
        LE scan is stopped successfully.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Scanning
        Priority: 0
        """
        input = {}
        test_result = self.validate_scan_settings_helper(input, self.droid)
        if not test_result:
            self.log.error("Could not setup ble scanner.")
            return test_result
        self.droid.bleStartBleScan(self.filter_list, self.scan_settings_index,
                                   self.scan_callback)
        try:
            self.droid.bleStopBleScan(self.scan_callback)
        except BleScanResultsError as error:
            self.log.error(str(error))
            test_result = False
        return test_result

    @BluetoothBaseTest.bt_test_wrap
    def test_scan_settings_callback_type_all_matches(self):
        """Test LE scan settings callback type all matches.

        Test scan settings callback type all matches.

        Steps:
        1. Validate the scan settings callback type with all other settings set
        to their respective defaults.

        Expected Result:
        Expected Scan settings should match found scan settings.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Scanning
        Priority: 1
        """
        input = {}
        input["ScanSettings"] = (
            ScanSettingsCallbackType.CALLBACK_TYPE_ALL_MATCHES.value, 0,
            ScanSettingsScanMode.SCAN_MODE_LOW_POWER.value,
            ScanSettingsScanResultType.SCAN_RESULT_TYPE_FULL.value)
        return self.validate_scan_settings_helper(input, self.droid)

    @BluetoothBaseTest.bt_test_wrap
    def test_scan_settings_set_callback_type_first_match(self):
        """Test LE scan settings callback type first match

        Test scan settings callback type first match.

        Steps:
        1. Validate the scan settings callback type with all other settings set
        to their respective defaults.

        Expected Result:
        Expected Scan settings should match found scan settings.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Scanning
        Priority: 1
        """
        input = {}
        input["ScanSettings"] = (
            ScanSettingsCallbackType.CALLBACK_TYPE_FIRST_MATCH.value, 0,
            ScanSettingsScanMode.SCAN_MODE_LOW_POWER.value,
            ScanSettingsScanResultType.SCAN_RESULT_TYPE_FULL.value)
        test_result = self.validate_scan_settings_helper(input, self.droid)
        return test_result

    @BluetoothBaseTest.bt_test_wrap
    def test_scan_settings_set_callback_type_match_lost(self):
        """Test LE scan settings callback type match lost.

        Test scan settings callback type match lost.

        Steps:
        1. Validate the scan settings callback type with all other settings set
        to their respective defaults.

        Expected Result:
        Expected Scan settings should match found scan settings.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Scanning
        Priority: 1
        """
        input = {}
        input["ScanSettings"] = (
            ScanSettingsCallbackType.CALLBACK_TYPE_MATCH_LOST.value, 0,
            ScanSettingsScanMode.SCAN_MODE_LOW_POWER.value,
            ScanSettingsScanResultType.SCAN_RESULT_TYPE_FULL.value)
        test_result = self.validate_scan_settings_helper(input, self.droid)
        return test_result

    @BluetoothBaseTest.bt_test_wrap
    def test_scan_settings_set_invalid_callback_type(self):
        """Test LE scan settings invalid callback type.

        Test scan settings invalid callback type -1.

        Steps:
        1. Build a LE ScanSettings object with an invalid callback type.

        Expected Result:
        Api should fail to build object.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Scanning
        Priority: 2
        """
        input = {}
        input["ScanSettings"] = (
            -1, 0, ScanSettingsScanMode.SCAN_MODE_LOW_POWER.value,
            ScanSettingsScanResultType.SCAN_RESULT_TYPE_FULL.value)
        test_result = self.validate_scan_settings_helper(input, self.droid)
        return not test_result

    @BluetoothBaseTest.bt_test_wrap
    def test_scan_settings_set_scan_mode_low_power(self):
        """Test LE scan settings scan mode low power mode.

        Test scan settings scan mode low power.

        Steps:
        1. Validate the scan settings scan mode with all other settings set to
        their respective defaults.

        Expected Result:
        Expected Scan settings should match found scan settings.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Scanning
        Priority: 1
        """
        input = {}
        input["ScanSettings"] = (
            ScanSettingsCallbackType.CALLBACK_TYPE_ALL_MATCHES.value, 0,
            ScanSettingsScanMode.SCAN_MODE_LOW_POWER.value,
            ScanSettingsScanResultType.SCAN_RESULT_TYPE_FULL.value)
        test_result = self.validate_scan_settings_helper(input, self.droid)
        return test_result

    @BluetoothBaseTest.bt_test_wrap
    def test_scan_settings_set_scan_mode_balanced(self):
        """Test LE scan settings scan mode balanced.

        Test scan settings scan mode balanced.

        Steps:
        1. Validate the scan settings scan mode with all other settings set to
        their respective defaults.

        Expected Result:
        Expected Scan settings should match found scan settings.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Scanning
        Priority: 1
        """
        input = {}
        input["ScanSettings"] = (
            ScanSettingsCallbackType.CALLBACK_TYPE_ALL_MATCHES.value, 0,
            ScanSettingsScanMode.SCAN_MODE_BALANCED.value,
            ScanSettingsScanResultType.SCAN_RESULT_TYPE_FULL.value)
        return self.validate_scan_settings_helper(input, self.droid)

    @BluetoothBaseTest.bt_test_wrap
    def test_scan_settings_set_scan_mode_low_latency(self):
        """Test LE scan settings scan mode low latency.

        Test scan settings scan mode low latency.

        Steps:
        1. Validate the scan settings scan mode with all other settings set to
        their respective defaults.

        Expected Result:
        Expected Scan settings should match found scan settings.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Scanning
        Priority: 1
        """
        input = {}
        input["ScanSettings"] = (
            ScanSettingsCallbackType.CALLBACK_TYPE_ALL_MATCHES.value, 0,
            ScanSettingsScanMode.SCAN_MODE_LOW_LATENCY.value,
            ScanSettingsScanResultType.SCAN_RESULT_TYPE_FULL.value)
        return self.validate_scan_settings_helper(input, self.droid)

    @BluetoothBaseTest.bt_test_wrap
    def test_scan_settings_set_invalid_scan_mode(self):
        """Test LE scan settings scan mode as an invalid value.
        Test scan settings invalid scan mode -2.
        Steps:
        1. Set the scan settings scan mode to -2.

        Expected Result:
        Building the ScanSettings object should fail.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Scanning
        Priority: 1
        """
        input = {}
        input["ScanSettings"] = (
            ScanSettingsCallbackType.CALLBACK_TYPE_ALL_MATCHES.value, 0, -2,
            ScanSettingsScanResultType.SCAN_RESULT_TYPE_FULL.value)
        return not self.validate_scan_settings_helper(input, self.droid)

    @BluetoothBaseTest.bt_test_wrap
    def test_scan_settings_set_report_delay_millis_min(self):
        """Test scan settings report delay millis as min value

        Test scan settings report delay millis min acceptable value.

        Steps:
        1. Validate the scan settings report delay millis with all other
        settings set to their respective defaults.

        Expected Result:
        Expected Scan settings should match found scan settings.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Scanning
        Priority: 2
        """
        input = {}
        input["ScanSettings"] = (
            ScanSettingsCallbackType.CALLBACK_TYPE_ALL_MATCHES.value,
            ScanSettingsReportDelaySeconds.MIN.value,
            ScanSettingsScanMode.SCAN_MODE_LOW_POWER.value,
            ScanSettingsScanResultType.SCAN_RESULT_TYPE_FULL.value)
        return self.validate_scan_settings_helper(input, self.droid)

    @BluetoothBaseTest.bt_test_wrap
    def test_scan_settings_set_report_delay_millis_min_plus_one(self):
        """Test scan settings report delay millis as min value plus one.

        Test scan settings report delay millis as min value plus one.

        Steps:
        1. Validate the scan settings report delay millis with all other
        settings set to their respective defaults.

        Expected Result:
        Expected Scan settings should match found scan settings.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Scanning
        Priority: 4
        """
        input = {}
        input["ScanSettings"] = (
            ScanSettingsCallbackType.CALLBACK_TYPE_ALL_MATCHES.value,
            ScanSettingsReportDelaySeconds.MIN.value + 1,
            ScanSettingsScanMode.SCAN_MODE_LOW_POWER.value,
            ScanSettingsScanResultType.SCAN_RESULT_TYPE_FULL.value)
        return self.validate_scan_settings_helper(input, self.droid)

    @BluetoothBaseTest.bt_test_wrap
    def test_scan_settings_set_report_delay_millis_max(self):
        """Test scan settings report delay millis as max value.

        Test scan settings report delay millis max value.

        Steps:
        1. Validate the scan settings report delay millis with all other
        settings set to their respective defaults.

        Expected Result:
        Expected Scan settings should match found scan settings.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Scanning
        Priority: 3
        """
        input = {}
        input["ScanSettings"] = (
            ScanSettingsCallbackType.CALLBACK_TYPE_ALL_MATCHES.value,
            ScanSettingsReportDelaySeconds.MAX.value,
            ScanSettingsScanMode.SCAN_MODE_LOW_POWER.value,
            ScanSettingsScanResultType.SCAN_RESULT_TYPE_FULL.value)
        return self.validate_scan_settings_helper(input, self.droid)

    @BluetoothBaseTest.bt_test_wrap
    def test_scan_settings_set_report_delay_millis_max_minus_one(self):
        """Test scan settings report delay millis as max value minus one.

        Test scan settings report delay millis max value - 1.

        Steps:
        1. Validate the scan settings report delay millis with all other
        settings set to their respective defaults.

        Expected Result:
        Expected Scan settings should match found scan settings.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Scanning
        Priority: 3
        """
        input = {}
        input["ScanSettings"] = (
            ScanSettingsCallbackType.CALLBACK_TYPE_ALL_MATCHES.value,
            ScanSettingsReportDelaySeconds.MAX.value - 1,
            ScanSettingsScanMode.SCAN_MODE_LOW_POWER.value,
            ScanSettingsScanResultType.SCAN_RESULT_TYPE_FULL.value)
        return self.validate_scan_settings_helper(input, self.droid)

    @BluetoothBaseTest.bt_test_wrap
    def test_scan_settings_set_invalid_report_delay_millis_min_minus_one(self):
        """Test scan settings report delay millis as an invalid value.

        Test scan settings invalid report delay millis min value - 1.

        Steps:
        1. Set scan settings report delay millis to min value -1.
        2. Build scan settings object.

        Expected Result:
        Building scan settings object should fail.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Scanning
        Priority: 2
        """
        droid = self.droid
        input = {}
        input["ScanSettings"] = (
            ScanSettingsCallbackType.CALLBACK_TYPE_ALL_MATCHES.value,
            ScanSettingsReportDelaySeconds.MIN.value - 1,
            ScanSettingsScanMode.SCAN_MODE_LOW_POWER.value,
            ScanSettingsScanResultType.SCAN_RESULT_TYPE_FULL.value)
        return not self.validate_scan_settings_helper(input, droid)

    @BluetoothBaseTest.bt_test_wrap
    def test_scan_settings_set_scan_result_type_full(self):
        """Test scan settings result type full.

        Test scan settings result type full.

        Steps:
        1. Validate the scan settings result type with all other settings
        set to their respective defaults.

        Expected Result:
        Expected Scan settings should match found scan settings.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Scanning
        Priority: 1
        """
        input = {}
        input["ScanSettings"] = (
            ScanSettingsCallbackType.CALLBACK_TYPE_ALL_MATCHES.value, 0,
            ScanSettingsScanMode.SCAN_MODE_LOW_POWER.value,
            ScanSettingsScanResultType.SCAN_RESULT_TYPE_FULL.value)
        return self.validate_scan_settings_helper(input, self.droid)

    @BluetoothBaseTest.bt_test_wrap
    def test_scan_settings_set_scan_result_type_abbreviated(self):
        """Test scan settings result type abbreviated.

        Test scan settings result type abbreviated.

        Steps:
        1. Validate the scan settings result type with all other settings
        set to their respective defaults.

        Expected Result:
        Expected Scan settings should match found scan settings.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Scanning
        Priority: 1
        """
        input = {}
        input["ScanSettings"] = (
            ScanSettingsCallbackType.CALLBACK_TYPE_ALL_MATCHES.value, 0,
            ScanSettingsScanMode.SCAN_MODE_LOW_POWER.value,
            ScanSettingsScanResultType.SCAN_RESULT_TYPE_ABBREVIATED.value)
        return self.validate_scan_settings_helper(input, self.droid)

    @BluetoothBaseTest.bt_test_wrap
    def test_scan_settings_set_invalid_scan_result_type(self):
        """Test scan settings result type as an invalid value.

        Test scan settings invalid result type -1.

        Steps:
        1. Set scan settings result type as an invalid value.
        2. Build scan settings object.

        Expected Result:
        Expected Scan settings should match found scan settings.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Scanning
        Priority: 2
        """
        input = {}
        input["ScanSettings"] = (
            ScanSettingsCallbackType.CALLBACK_TYPE_ALL_MATCHES.value, 0,
            ScanSettingsScanMode.SCAN_MODE_LOW_POWER.value, -1)
        return not self.validate_scan_settings_helper(input, self.droid)

    @BluetoothBaseTest.bt_test_wrap
    def test_scan_filter_set_device_name(self):
        """Test scan filter set valid device name.

        Test scan filter device name sl4atest.

        Steps:
        1. Validate the scan filter device name with all other settings
        set to their respective defaults.

        Expected Result:
        Expected Scan filter should match found scan settings.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Scanning
        Priority: 1
        """
        input = {}
        input['ScanFilterDeviceName'] = "sl4atest"
        return self.validate_scan_settings_helper(input, self.droid)

    @BluetoothBaseTest.bt_test_wrap
    def test_scan_filter_set_device_name_blank(self):
        """Test scan filter set blank device name.

        Test scan filter device name blank.

        Steps:
        1. Validate the scan filter device name with all other settings
        set to their respective defaults.

        Expected Result:
        Expected Scan filter should match found scan filter.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Scanning
        Priority: 1
        """
        droid = self.droid
        input = {}
        input['ScanFilterDeviceName'] = ""
        return self.validate_scan_settings_helper(input, droid)

    @BluetoothBaseTest.bt_test_wrap
    def test_scan_filter_set_device_name_special_chars(self):
        """Test scan filter set device name as special chars.

        Test scan filter device name special characters.

        Steps:
        1. Validate the scan filter device name with all other settings
        set to their respective defaults.

        Expected Result:
        Expected Scan filter should match found scan filter.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Scanning
        Priority: 1
        """
        input = {}
        input['ScanFilterDeviceName'] = "!@#$%^&*()\":<>/"
        return self.validate_scan_settings_helper(input, self.droid)

    @BluetoothBaseTest.bt_test_wrap
    def test_scan_filter_set_device_address(self):
        """Test scan filter set valid device address.

        Test scan filter device address valid.

        Steps:
        1. Validate the scan filter device address with all other settings
        set to their respective defaults.

        Expected Result:
        Expected Scan filter should match found scan filter.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Scanning
        Priority: 1
        """
        input = {}
        input['ScanFilterDeviceAddress'] = "01:02:03:AB:CD:EF"
        return self.validate_scan_settings_helper(input, self.droid)

    @BluetoothBaseTest.bt_test_wrap
    def test_scan_filter_set_invalid_device_address_lower_case(self):
        """Test scan filter set invalid device address.

        Test scan filter device address lower case.

        Steps:
        1. Set the scan filter address to an invalid, lowercase mac address

        Expected Result:
        Api to build scan filter should fail.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Scanning
        Priority: 2
        """
        input = {}
        input['ScanFilterDeviceAddress'] = "01:02:03:ab:cd:ef"
        return not self.validate_scan_settings_helper(input, self.droid)

    @BluetoothBaseTest.bt_test_wrap
    def test_scan_filter_set_invalid_device_address_blank(self):
        """Test scan filter set invalid device address.

        Test scan filter invalid device address blank.

        Steps:
        1. Set the scan filter address to an invalid, blank mac address

        Expected Result:
        Api to build scan filter should fail.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Scanning
        Priority: 2
        """
        input = {}
        input['ScanFilterDeviceAddress'] = ""
        test_result = self.validate_scan_settings_helper(input, self.droid)
        return not test_result

    @BluetoothBaseTest.bt_test_wrap
    def test_scan_filter_set_invalid_device_address_bad_format(self):
        """Test scan filter set badly formatted device address.

        Test scan filter badly formatted device address.

        Steps:
        1. Set the scan filter address to an invalid, blank mac address

        Expected Result:
        Api to build scan filter should fail.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Scanning
        Priority: 2
        """
        input = {}
        input['ScanFilterDeviceAddress'] = "10.10.10.10.10"
        return not self.validate_scan_settings_helper(input, self.droid)

    @BluetoothBaseTest.bt_test_wrap
    def test_scan_filter_set_invalid_device_address_bad_address(self):
        """Test scan filter device address as an invalid value.

        Test scan filter invalid device address invalid characters.

        Steps:
        1. Set a scan filter's device address as ZZ:ZZ:ZZ:ZZ:ZZ:ZZ

        Expected Result:
        Api to build the scan filter should fail.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Scanning
        Priority: 1
        """
        input = {}
        input['ScanFilterDeviceAddress'] = "ZZ:ZZ:ZZ:ZZ:ZZ:ZZ"
        return not self.validate_scan_settings_helper(input, self.droid)

    @BluetoothBaseTest.bt_test_wrap
    def test_scan_filter_set_manufacturer_id_data(self):
        """Test scan filter manufacturer data.

        Test scan filter manufacturer data with a valid input.

        Steps:
        1. Validate the scan filter manufacturer id with all other settings
        set to their respective defaults.

        Expected Result:
        Expected Scan filter should match found scan filter.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Scanning
        Priority: 1
        """
        expected_manufacturer_id = 0
        expected_manufacturer_data = "1,2,1,3,4,5,6"
        input = {}
        input['ScanFilterManufacturerDataId'] = expected_manufacturer_id
        input['ScanFilterManufacturerData'] = expected_manufacturer_data
        return self.validate_scan_settings_helper(input, self.droid)

    @BluetoothBaseTest.bt_test_wrap
    def test_scan_filter_set_manufacturer_id_data_mask(self):
        """Test scan filter manufacturer data mask.

        Test scan filter manufacturer data with a valid data mask.

        Steps:
        1. Validate the scan filter manufacturer id with all other settings
        set to their respective defaults.

        Expected Result:
        Expected Scan filter should match found scan filter.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Scanning
        Priority: 1
        """
        expected_manufacturer_id = 1
        expected_manufacturer_data = "1"
        expected_manufacturer_data_mask = "1,2,1,3,4,5,6"
        input = {}
        input['ScanFilterManufacturerDataId'] = expected_manufacturer_id
        input['ScanFilterManufacturerData'] = expected_manufacturer_data
        input[
            'ScanFilterManufacturerDataMask'] = expected_manufacturer_data_mask
        return self.validate_scan_settings_helper(input, self.droid)

    @BluetoothBaseTest.bt_test_wrap
    def test_scan_filter_set_manufacturer_max_id(self):
        """Test scan filter manufacturer data id.

        Test scan filter manufacturer data max id.

        Steps:
        1. Validate the scan filter manufacturer id with all other settings
        set to their respective defaults.

        Expected Result:
        Expected Scan filter should match found scan filter.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Scanning
        Priority: 2
        """
        expected_manufacturer_id = 2147483647
        expected_manufacturer_data = "1,2,1,3,4,5,6"
        input = {}
        input['ScanFilterManufacturerDataId'] = expected_manufacturer_id
        input['ScanFilterManufacturerData'] = expected_manufacturer_data
        return self.validate_scan_settings_helper(input, self.droid)

    @BluetoothBaseTest.bt_test_wrap
    def test_scan_filter_set_manufacturer_data_empty(self):
        """Test scan filter empty manufacturer data.

        Test scan filter manufacturer data as empty but valid manufacturer data.

        Steps:
        1. Validate the scan filter manufacturer id with all other settings
        set to their respective defaults.

        Expected Result:
        Expected Scan filter should match found scan filter.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Scanning
        Priority: 2
        """
        expected_manufacturer_id = 1
        expected_manufacturer_data = ""
        input = {}
        input['ScanFilterManufacturerDataId'] = expected_manufacturer_id
        input['ScanFilterManufacturerData'] = expected_manufacturer_data
        return self.validate_scan_settings_helper(input, self.droid)

    @BluetoothBaseTest.bt_test_wrap
    def test_scan_filter_set_manufacturer_data_mask_empty(self):
        """Test scan filter empty manufacturer data mask.

        Test scan filter manufacturer mask empty.

        Steps:
        1. Validate the scan filter manufacturer id with all other settings
        set to their respective defaults.

        Expected Result:
        Expected Scan filter should match found scan filter.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Scanning
        Priority: 1
        """
        expected_manufacturer_id = 1
        expected_manufacturer_data = "1,2,1,3,4,5,6"
        expected_manufacturer_data_mask = ""
        input = {}
        input['ScanFilterManufacturerDataId'] = expected_manufacturer_id
        input['ScanFilterManufacturerData'] = expected_manufacturer_data
        input[
            'ScanFilterManufacturerDataMask'] = expected_manufacturer_data_mask
        return self.validate_scan_settings_helper(input, self.droid)

    @BluetoothBaseTest.bt_test_wrap
    def test_scan_filter_set_invalid_manufacturer_min_id_minus_one(self):
        """Test scan filter invalid manufacturer data.

        Test scan filter invalid manufacturer id min value - 1.

        Steps:
        1. Set the scan filters manufacturer id to -1.
        2. Build the scan filter.

        Expected Result:
        Api to build the scan filter should fail.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Scanning
        Priority: 2
        """
        expected_manufacturer_id = -1
        expected_manufacturer_data = "1,2,1,3,4,5,6"
        input = {}
        input['ScanFilterManufacturerDataId'] = expected_manufacturer_id
        input['ScanFilterManufacturerData'] = expected_manufacturer_data
        return not self.validate_scan_settings_helper(input, self.droid)

    @BluetoothBaseTest.bt_test_wrap
    def test_scan_filter_set_service_uuid(self):
        """Test scan filter set valid service uuid.

        Test scan filter service uuid.

        Steps:
        1. Validate the scan filter service uuid with all other settings
        set to their respective defaults.

        Expected Result:
        Expected Scan filter should match found scan filter.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Scanning
        Priority: 1
        """
        expected_service_uuid = "00000000-0000-1000-8000-00805F9B34FB"
        expected_service_mask = "00000000-0000-1000-8000-00805F9B34FB"
        input = {}
        input['ScanFilterServiceUuid'] = expected_service_uuid
        input['ScanFilterServiceMask'] = expected_service_mask
        return self.validate_scan_settings_helper(input, self.droid)

    @BluetoothBaseTest.bt_test_wrap
    def test_scan_filter_service_uuid_p_service(self):
        """Test scan filter service uuid.

        Test scan filter service uuid p service

        Steps:
        1. Validate the scan filter service uuid with all other settings
        set to their respective defaults.

        Expected Result:
        Expected Scan filter should match found scan filter.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Scanning
        Priority: 2
        """
        expected_service_uuid = Uuids.P_Service.value
        expected_service_mask = "00000000-0000-1000-8000-00805F9B34FB"
        self.log.debug("Step 1: Setup environment.")

        input = {}
        input['ScanFilterServiceUuid'] = expected_service_uuid
        input['ScanFilterServiceMask'] = expected_service_mask
        return self.validate_scan_settings_helper(input, self.droid)

    @BluetoothBaseTest.bt_test_wrap
    def test_classic_ble_scan_with_service_uuids_p(self):
        """Test classic LE scan with valid service uuid.

        Test classic ble scan with scan filter service uuid p service uuids.

        Steps:
        1. Validate the scan filter service uuid with all other settings
        set to their respective defaults.
        2. Start classic ble scan.
        3. Stop classic ble scan

        Expected Result:
        Expected Scan filter should match found scan filter.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Scanning
        Priority: 1
        """

        droid = self.droid
        service_uuid_list = [Uuids.P_Service.value]
        scan_callback = droid.bleGenLeScanCallback()
        return self.verify_classic_ble_scan_with_service_uuids(
            droid, scan_callback, service_uuid_list)

    @BluetoothBaseTest.bt_test_wrap
    def test_classic_ble_scan_with_service_uuids_hr(self):
        """Test classic LE scan with valid service uuid.

        Test classic ble scan with scan filter service uuid hr service

        Steps:
        1. Validate the scan filter service uuid with all other settings
        set to their respective defaults.
        2. Start classic ble scan.
        3. Stop classic ble scan

        Expected Result:
        Expected Scan filter should match found scan filter.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Scanning
        Priority: 1
        """
        droid = self.droid
        service_uuid_list = [Uuids.HR_SERVICE.value]
        scan_callback = droid.bleGenLeScanCallback()
        return self.verify_classic_ble_scan_with_service_uuids(
            droid, scan_callback, service_uuid_list)

    @BluetoothBaseTest.bt_test_wrap
    def test_classic_ble_scan_with_service_uuids_empty_uuid_list(self):
        """Test classic LE scan with empty but valid uuid list.

        Test classic ble scan with service uuids as empty list.

        Steps:
        1. Validate the scan filter service uuid with all other settings
        set to their respective defaults.
        2. Start classic ble scan.
        3. Stop classic ble scan

        Expected Result:
        Expected Scan filter should match found scan filter.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Scanning
        Priority: 1
        """
        droid = self.droid
        service_uuid_list = []
        scan_callback = droid.bleGenLeScanCallback()
        return self.verify_classic_ble_scan_with_service_uuids(
            droid, scan_callback, service_uuid_list)

    @BluetoothBaseTest.bt_test_wrap
    def test_classic_ble_scan_with_service_uuids_hr_and_p(self):
        """Test classic LE scan with multiple service uuids.

        Test classic ble scan with service uuids a list of hr and p service.

        Steps:
        1. Validate the scan filter service uuid with all other settings
        set to their respective defaults.
        2. Start classic ble scan.
        3. Stop classic ble scan

        Expected Result:
        Expected Scan filter should match found scan filter.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Scanning
        Priority: 1
        """
        droid = self.droid
        service_uuid_list = [Uuids.HR_SERVICE.value, Uuids.P_Service.value]
        scan_callback = droid.bleGenLeScanCallback()
        return self.verify_classic_ble_scan_with_service_uuids(
            droid, scan_callback, service_uuid_list)

    def verify_classic_ble_scan_with_service_uuids(self, droid, scan_callback,
                                                   service_uuid_list):

        test_result = True
        try:
            test_result = droid.bleStartClassicBleScanWithServiceUuids(
                scan_callback, service_uuid_list)
        except BleScanResultsError as error:
            self.log.error(str(error))
            return False
        droid.bleStopClassicBleScan(scan_callback)
        if not test_result:
            self.log.error(
                "Start classic ble scan with service uuids return false "
                "boolean value.")
            return False
        return True
