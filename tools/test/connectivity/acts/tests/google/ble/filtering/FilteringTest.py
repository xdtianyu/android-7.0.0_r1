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

import itertools as it
import pprint
import time

from queue import Empty
from acts.test_utils.bt.BluetoothBaseTest import BluetoothBaseTest
from acts.test_utils.bt.BleEnum import AdvertiseSettingsAdvertiseMode
from acts.test_utils.bt.BleEnum import AdvertiseSettingsAdvertiseTxPower
from acts.test_utils.bt.BleEnum import JavaInteger
from acts.test_utils.bt.BleEnum import ScanSettingsScanMode
from acts.test_utils.bt.BleEnum import ScanSettingsScanMode
from acts.test_utils.bt.bt_test_utils import adv_fail
from acts.test_utils.bt.bt_test_utils import adv_succ
from acts.test_utils.bt.bt_test_utils import generate_ble_advertise_objects
from acts.test_utils.bt.bt_test_utils import get_advanced_droid_list
from acts.test_utils.bt.bt_test_utils import reset_bluetooth
from acts.test_utils.bt.bt_test_utils import scan_result


class FilteringTest(BluetoothBaseTest):
    default_timeout = 30

    valid_filter_suite = [
        {
            'include_tx_power_level': True
        },
        {
            'filter_device_address': True
        },
        {
            'manufacturer_specific_data_id': 1,
            'manufacturer_specific_data': "1"
        },
        {
            'manufacturer_specific_data_id': 1,
            'manufacturer_specific_data': "14,0,54,0,0,0,0,0"
        },
        {
            'manufacturer_specific_data_id': 1,
            'manufacturer_specific_data': "1",
            'manufacturer_specific_data_mask': "1"
        },
        {
            'service_data_uuid': "0000110A-0000-1000-8000-00805F9B34FB",
            'service_data': "1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,26,17,18,19,"
                            "20,21,22,23,24"
        },
        {
            'service_data_uuid': "0000110B-0000-1000-8000-00805F9B34FB",
            'service_data': "13"
        },
        {
            'service_data_uuid': "0000110C-0000-1000-8000-00805F9B34FB",
            'service_data': "11,14,50"
        },
        {
            'service_data_uuid': "0000110D-0000-1000-8000-00805F9B34FB",
            'service_data': "16,22,11"
        },
        {
            'service_data_uuid': "0000110E-0000-1000-8000-00805F9B34FB",
            'service_data': "2,9,54"
        },
        {
            'service_data_uuid': "0000110F-0000-1000-8000-00805F9B34FB",
            'service_data': "69,11,50"
        },
        {
            'service_data_uuid': "00001101-0000-1000-8000-00805F9B34FB",
            'service_data': "12,11,21"
        },
        {
            'service_data_uuid': "00001102-0000-1000-8000-00805F9B34FB",
            'service_data': "12,12,44"
        },
        {
            'service_data_uuid': "00001103-0000-1000-8000-00805F9B34FB",
            'service_data': "4,54,1"
        },
        {
            'service_data_uuid': "00001104-0000-1000-8000-00805F9B34FB",
            'service_data': "33,22,44"
        },
        {
            'service_uuid': "00000000-0000-1000-8000-00805f9b34fb",
            'service_mask': "00000000-0000-1000-8000-00805f9b34fb",
        },
        {
            'service_uuid': "FFFFFFFF-0000-1000-8000-00805f9b34fb",
            'service_mask': "00000000-0000-1000-8000-00805f9b34fb",
        },
        {
            'service_uuid': "3846D7A0-69C8-11E4-BA00-0002A5D5C51B",
            'service_mask': "00000000-0000-1000-8000-00805f9b34fb",
        },
        {
            'include_device_name': True
        },
    ]

    valid_filter_variants = {
        'include_tx_power_level': [True, False],
        'manufacturer_specific_data_id': [1, 2, 65535],
        'manufacturer_specific_data': ["1", "1,2", "127"],
        'service_data_uuid': ["00000000-0000-1000-8000-00805f9b34fb"],
        'service_data': ["1,2,3", "1", "127"],
        'include_device_name': [False, True],
    }

    multi_manufacturer_specific_data_suite = {
        'manufacturer_specific_data_list': [[(1, "1"), (2, "2"),
                                             (65535, "127")]],
    }

    settings_in_effect_variants = {
        "mode": [
            AdvertiseSettingsAdvertiseMode.ADVERTISE_MODE_BALANCED.value,
            AdvertiseSettingsAdvertiseMode.ADVERTISE_MODE_LOW_LATENCY.value,
        ],
        "tx_power_level": [
            AdvertiseSettingsAdvertiseTxPower.ADVERTISE_TX_POWER_HIGH.value,
            AdvertiseSettingsAdvertiseTxPower.ADVERTISE_TX_POWER_LOW.value,
            AdvertiseSettingsAdvertiseTxPower.ADVERTISE_TX_POWER_ULTRA_LOW.
            value,
            AdvertiseSettingsAdvertiseTxPower.ADVERTISE_TX_POWER_MEDIUM.value,
        ],
        "is_connectable": [True, False],
        "scan_mode": [ScanSettingsScanMode.SCAN_MODE_LOW_LATENCY.value,
                      ScanSettingsScanMode.SCAN_MODE_OPPORTUNISTIC.value,
                      ScanSettingsScanMode.SCAN_MODE_BALANCED.value,
                      ScanSettingsScanMode.SCAN_MODE_LOW_POWER.value, ]
    }

    default_callback = 1
    default_is_connectable = True
    default_advertise_mode = 0
    default_tx_power_level = 2

    def _get_combinations(self, t):
        varNames = sorted(t)
        return ([dict(zip(varNames, prod))
                 for prod in it.product(*(t[varName]
                                          for varName in varNames))])

    def __init__(self, controllers):
        BluetoothBaseTest.__init__(self, controllers)
        self.droid_list = get_advanced_droid_list(self.android_devices)
        self.scn_ad = self.android_devices[0]
        self.adv_ad = self.android_devices[1]
        if self.droid_list[1]['max_advertisements'] == 0:
            self.tests = ()
            return
        self.log.info("Scanner device model: {}".format(
            self.scn_ad.droid.getBuildModel()))
        self.log.info("Advertiser device model: {}".format(
            self.adv_ad.droid.getBuildModel()))

    def blescan_verify_onfailure_event_handler(self, event):
        self.log.debug("Verifying {} event".format(adv_fail))
        self.log.debug(pprint.pformat(event))
        return event

    def blescan_verify_onscanresult_event_handler(self, event, filters):
        test_result = True
        self.log.debug("Verifying onScanResult event: {}".format(event))
        callback_type = event['data']['CallbackType']
        if 'callback_type' in filters.keys():
            if filters['callback_type'] != callback_type:
                self.log.error("Expected callback type: {}, Found callback "
                               "type: {}".format(filters['callback_type'],
                                                 callback_type))
            test_result = False
        elif self.default_callback != callback_type:
            self.log.error("Expected callback type: {}, Found callback type: "
                           "{}".format(self.default_callback, callback_type))
            test_result = False
        if 'include_device_name' in filters.keys() and filters[
                'include_device_name'] is not False:
            if event['data']['Result']['deviceName'] != filters[
                    'include_device_name']:
                self.log.error(
                    "Expected device name: {}, Found device name: {}"
                    .format(filters['include_device_name'], event['data'][
                        'Result']['deviceName']))

                test_result = False
        elif 'deviceName' in event['data']['Result'].keys():
            self.log.error(
                "Device name was found when it wasn't meant to be included.")
            test_result = False
        if ('include_tx_power_level' in filters.keys() and filters[
                'include_tx_power_level'] is not False):
            if not event['data']['Result']['txPowerLevel']:
                self.log.error(
                    "Expected to find tx power level in event but found none.")
                test_result = False
        if not event['data']['Result']['rssi']:
            self.log.error("Expected rssi in the advertisement, found none.")
            test_result = False
        if not event['data']['Result']['timestampNanos']:
            self.log.error("Expected rssi in the advertisement, found none.")
            test_result = False
        return test_result

    def bleadvertise_verify_onsuccess_handler(self, event, settings_in_effect):
        self.log.debug("Verifying {} event".format(adv_succ))
        test_result = True
        if 'is_connectable' in settings_in_effect.keys():
            if (event['data']['SettingsInEffect']['isConnectable'] !=
                    settings_in_effect['is_connectable']):
                self.log.error("Expected is connectable value: {}, Actual is "
                               "connectable value:".format(settings_in_effect[
                                   'is_connectable'], event['data'][
                                       'SettingsInEffect']['isConnectable']))
                test_result = False
        elif (event['data']['SettingsInEffect']['isConnectable'] !=
              self.default_is_connectable):
            self.log.error(
                "Default value for isConnectable did not match what was found.")
            test_result = False
        if 'mode' in settings_in_effect.keys():
            if (event['data']['SettingsInEffect']['mode'] !=
                    settings_in_effect['mode']):
                self.log.error("Expected mode value: {}, Actual mode value: {}"
                               .format(settings_in_effect['mode'], event[
                                   'data']['SettingsInEffect']['mode']))
                test_result = False
        elif (event['data']['SettingsInEffect']['mode'] !=
              self.default_advertise_mode):
            self.log.error(
                "Default value for filtering mode did not match what was "
                "found.")
            test_result = False
        if 'tx_power_level' in settings_in_effect.keys():
            if (event['data']['SettingsInEffect']['txPowerLevel'] ==
                    JavaInteger.MIN.value):
                self.log.error("Expected tx power level was not meant to be: "
                               "{}".format(JavaInteger.MIN.value))
                test_result = False
        elif (event['data']['SettingsInEffect']['txPowerLevel'] !=
              self.default_tx_power_level):
            self.log.error(
                "Default value for tx power level did not match what"
                " was found.")
            test_result = False
        return test_result

    def _magic(self, params):
        (filters, settings_in_effect) = params
        test_result = True

        self.log.debug("Settings in effect: {}".format(pprint.pformat(
            settings_in_effect)))
        self.log.debug("Filters:".format(pprint.pformat(filters)))
        if 'is_connectable' in settings_in_effect.keys():
            self.log.debug("Setting advertisement is_connectable to {}".format(
                settings_in_effect['is_connectable']))
            self.adv_ad.droid.bleSetAdvertiseSettingsIsConnectable(
                settings_in_effect['is_connectable'])
        if 'mode' in settings_in_effect.keys():
            self.log.debug("Setting advertisement mode to {}"
                           .format(settings_in_effect['mode']))
            self.adv_ad.droid.bleSetAdvertiseSettingsAdvertiseMode(
                settings_in_effect['mode'])
        if 'tx_power_level' in settings_in_effect.keys():
            self.log.debug("Setting advertisement tx_power_level to {}".format(
                settings_in_effect['tx_power_level']))
            self.adv_ad.droid.bleSetAdvertiseSettingsTxPowerLevel(
                settings_in_effect['tx_power_level'])
        filter_list = self.scn_ad.droid.bleGenFilterList()
        if ('include_device_name' in filters.keys() and
                filters['include_device_name'] is not False):

            self.log.debug("Setting advertisement include_device_name to {}"
                           .format(filters['include_device_name']))
            self.adv_ad.droid.bleSetAdvertiseDataIncludeDeviceName(True)
            filters['include_device_name'] = (
                self.adv_ad.droid.bluetoothGetLocalName())
            self.log.debug("Setting scanner include_device_name to {}".format(
                filters['include_device_name']))
            self.scn_ad.droid.bleSetScanFilterDeviceName(filters[
                'include_device_name'])
        else:
            self.log.debug(
                "Setting advertisement include_device_name to False")
            self.adv_ad.droid.bleSetAdvertiseDataIncludeDeviceName(False)
        if ('include_tx_power_level' in filters.keys() and filters[
                'include_tx_power_level'] is not False):
            self.log.debug(
                "Setting advertisement include_tx_power_level to True")
            self.adv_ad.droid.bleSetAdvertiseDataIncludeTxPowerLevel(True)
        if 'manufacturer_specific_data_id' in filters.keys():
            if 'manufacturer_specific_data_mask' in filters.keys():
                self.adv_ad.droid.bleAddAdvertiseDataManufacturerId(
                    filters['manufacturer_specific_data_id'],
                    filters['manufacturer_specific_data'])
                self.scn_ad.droid.bleSetScanFilterManufacturerData(
                    filters['manufacturer_specific_data_id'],
                    filters['manufacturer_specific_data'],
                    filters['manufacturer_specific_data_mask'])
            else:
                self.adv_ad.droid.bleAddAdvertiseDataManufacturerId(
                    filters['manufacturer_specific_data_id'],
                    filters['manufacturer_specific_data'])
                self.scn_ad.droid.bleSetScanFilterManufacturerData(
                    filters['manufacturer_specific_data_id'],
                    filters['manufacturer_specific_data'])
        if 'service_data' in filters.keys():
            self.adv_ad.droid.bleAddAdvertiseDataServiceData(
                filters['service_data_uuid'], filters['service_data'])
            self.scn_ad.droid.bleSetScanFilterServiceData(
                filters['service_data_uuid'], filters['service_data'])
        if 'manufacturer_specific_data_list' in filters.keys():
            for pair in filters['manufacturer_specific_data_list']:
                (manu_id, manu_data) = pair
                self.adv_ad.droid.bleAddAdvertiseDataManufacturerId(manu_id,
                                                                    manu_data)
        if 'service_mask' in filters.keys():
            self.scn_ad.droid.bleSetScanFilterServiceUuid(
                filters['service_uuid'].upper(), filters['service_mask'])
            self.adv_ad.droid.bleSetAdvertiseDataSetServiceUuids([filters[
                'service_uuid'].upper()])
        elif 'service_uuid' in filters.keys():
            self.scn_ad.droid.bleSetScanFilterServiceUuid(filters[
                'service_uuid'])
            self.adv_ad.droid.bleSetAdvertiseDataSetServiceUuids([filters[
                'service_uuid']])
        self.scn_ad.droid.bleBuildScanFilter(filter_list)
        advertise_callback, advertise_data, advertise_settings = (
            generate_ble_advertise_objects(self.adv_ad.droid))
        if ('scan_mode' in settings_in_effect and
                settings_in_effect['scan_mode'] !=
                ScanSettingsScanMode.SCAN_MODE_OPPORTUNISTIC.value):
            self.scn_ad.droid.bleSetScanSettingsScanMode(settings_in_effect[
                'scan_mode'])
        else:
            self.scn_ad.droid.bleSetScanSettingsScanMode(
                ScanSettingsScanMode.SCAN_MODE_LOW_LATENCY.value)
        scan_settings = self.scn_ad.droid.bleBuildScanSetting()
        scan_callback = self.scn_ad.droid.bleGenScanCallback()
        self.scn_ad.droid.bleStartBleScan(filter_list, scan_settings,
                                          scan_callback)
        opportunistic = False
        scan_settings2, scan_callback2 = None, None
        if ('scan_mode' in settings_in_effect and
                settings_in_effect['scan_mode'] ==
                ScanSettingsScanMode.SCAN_MODE_OPPORTUNISTIC.value):
            opportunistic = True
            scan_settings2 = self.scn_ad.droid.bleBuildScanSetting()
            scan_callback2 = self.scn_ad.droid.bleGenScanCallback()
            self.scn_ad.droid.bleStartBleScan(filter_list, scan_settings2,
                                              scan_callback2)
            self.scn_ad.droid.bleSetScanSettingsScanMode(
                ScanSettingsScanMode.SCAN_MODE_OPPORTUNISTIC.value)
        self.adv_ad.droid.bleStartBleAdvertising(
            advertise_callback, advertise_data, advertise_settings)
        expected_advertise_event_name = adv_succ.format(advertise_callback)
        self.log.debug(expected_advertise_event_name)
        advertise_worker = self.adv_ad.ed.handle_event(
            self.bleadvertise_verify_onsuccess_handler,
            expected_advertise_event_name, ([settings_in_effect]),
            self.default_timeout)
        try:
            test_result = advertise_worker.result(self.default_timeout)
        except Empty as error:
            self.log.error("Test failed with Empty error: {}".format(error))
            return False
        expected_scan_event_name = scan_result.format(scan_callback)
        worker = self.scn_ad.ed.handle_event(
            self.blescan_verify_onscanresult_event_handler,
            expected_scan_event_name, ([filters]), self.default_timeout)
        try:
            finished = False
            start_time = time.time()
            while (time.time() < start_time + self.default_timeout and
                   not finished):

                test_result = worker.result(self.default_timeout)
                if test_result:
                    finished = True
        except Empty as error:
            test_result = False
            self.log.error("No scan result found: {}".format(error))
        if opportunistic:
            expected_scan_event_name = scan_result.format(scan_callback2)
            worker = self.scn_ad.ed.handle_event(
                self.blescan_verify_onscanresult_event_handler,
                expected_scan_event_name, ([filters]), self.default_timeout)
            try:
                worker.result(self.default_timeout)
            except Empty:
                self.log.error("Failure to find event on opportunistic scan.")
            self.scn_ad.droid.bleStopBleScan(scan_callback2)
        self.adv_ad.droid.bleStopBleAdvertising(advertise_callback)
        self.scn_ad.droid.bleStopBleScan(scan_callback)
        return test_result

    @BluetoothBaseTest.bt_test_wrap
    def test_default_advertisement(self):
        """Test a default advertisement.

        Test that a default advertisement is found and matches corresponding
        settings.

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

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 2
        """
        filters = {}
        settings_in_effect = {}
        params = (filters, settings_in_effect)
        return self._magic(params)

    @BluetoothBaseTest.bt_test_wrap
    def test_settings_in_effect_suite(self):
        """Test combinations of settings with scanning and advertising.

        Test combinations of valid advertising modes, tx power, is connectable,
        and scan modes.

        Steps:
        1. Generate testcases of the combination of settings_in_effect_variants
        dictionary. This involves setting scan settings and advertising
        settings.

        Expected Result:
        Scan filters match advertising settings and advertisements are found.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 1
        """
        settings = self._get_combinations(self.settings_in_effect_variants)
        filters = [{"include_device_name": True}]
        params = list(it.product(filters, settings))
        failed = self.run_generated_testcases(self._magic,
                                              params,
                                              tag="settings_in_effect_suite")
        if failed:
            return False
        return True

    @BluetoothBaseTest.bt_test_wrap
    def test_filters_suite(self):
        """Test combinations of settings with scanning and advertising.

        Test combinations of valid advertisement data and scan settings.

        Steps:
        1. Generate testcases of the combination of valid_filter_variants and
        settings dictionaries. This involves setting scan settings and
        advertising settings.

        Expected Result:
        Scan filters match advertising settings and advertisements are found.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 1
        """
        valid_filter_suit = self._get_combinations(self.valid_filter_variants)
        settings = [
            {'mode':
             AdvertiseSettingsAdvertiseMode.ADVERTISE_MODE_LOW_LATENCY.value}
        ]
        params = list(it.product(valid_filter_suit, settings))
        failed = self.run_generated_testcases(self._magic,
                                              params,
                                              tag="filters_suite")
        if failed:
            return False
        return True

    @BluetoothBaseTest.bt_test_wrap
    def test_filters_suite_opportunistic_scan(self):
        """Test combinations of settings with opportunistic scanning.

        Test combinations of valid advertisement data and scan settings. This
        emphasises scan mode opportunistic.

        Steps:
        1. Generate testcases of the combination of valid_filter_suite and
        settings dictionaries. This involves setting scan settings and
        advertising settings.

        Expected Result:
        Scan filters match advertising settings and advertisements are found.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning, Opportunistic Scan
        Priority: 1
        """
        reset_bluetooth(self.android_devices)
        valid_filter_suit = self._get_combinations(self.valid_filter_variants)
        settings = [
            {'mode':
             AdvertiseSettingsAdvertiseMode.ADVERTISE_MODE_LOW_LATENCY.value,
             'scan_mode': ScanSettingsScanMode.SCAN_MODE_LOW_LATENCY.value}
        ]
        params = list(it.product(valid_filter_suit, settings))
        failed = self.run_generated_testcases(self._magic,
                                              params,
                                              tag="filters_suite")
        if failed:
            return False
        return True

    @BluetoothBaseTest.bt_test_wrap
    def test_valid_filters(self):
        """Test combinations of settings with scanning and advertising.

        Test combinations of valid advertisement data and scan settings.

        Steps:
        1. Generate testcases of the combination of valid_filters and
        settings dictionaries. This involves setting scan settings and
        advertising settings.

        Expected Result:
        Scan filters match advertising settings and advertisements are found.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 1
        """
        reset_bluetooth(self.android_devices)
        settings = [
            {'mode':
             AdvertiseSettingsAdvertiseMode.ADVERTISE_MODE_LOW_LATENCY.value}
        ]
        params = list(it.product(self.valid_filter_suite, settings))
        failed = self.run_generated_testcases(self._magic,
                                              params,
                                              tag="valid_filters")
        if failed:
            return False
        return True

    @BluetoothBaseTest.bt_test_wrap
    def test_valid_filters_opportunistic_scan(self):
        """Test combinations of settings with opportunistic scanning.

        Test combinations of valid advertisement data and scan settings. This
        emphasises scan mode opportunistic.

        Steps:
        1. Generate testcases of the combination of valid_filter_suite and
        settings dictionaries. This involves setting scan settings and
        advertising settings.

        Expected Result:
        Scan filters match advertising settings and advertisements are found.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning, Opportunistic Scan
        Priority: 1
        """
        settings = [
            {'mode':
             AdvertiseSettingsAdvertiseMode.ADVERTISE_MODE_LOW_LATENCY.value,
             'scan_mode': ScanSettingsScanMode.SCAN_MODE_LOW_LATENCY.value}
        ]
        params = list(it.product(self.valid_filter_suite, settings))
        failed = self.run_generated_testcases(self._magic,
                                              params,
                                              tag="valid_filters")
        if failed:
            return False
        return True

    @BluetoothBaseTest.bt_test_wrap
    def test_non_connectable_advertise_data(self):
        """Test non connectable advertisement data.

        Non-connectable advertisement data does not include the AD flags in
        the advertisement giving back more data to the overall advertisement
        data size.

        Steps:
        1. Create a large advertisement data object.
        2. Set isConnectable to false.
        3. Build advertising objects.
        4. Start scanning.
        5. Start advertising.
        6. Find advertisement and verify data.

        Expected Result:
        Scan filters match advertising settings and advertisements are found.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Filtering, Scanning
        Priority: 1
        """
        settings = {'is_connectable': False}
        filters = {
            'service_data_uuid': "0000110A-0000-1000-8000-00805F9B34FB",
            'service_data': "1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,26,17,18,19,"
                            "20,21,22,23,24,25,26,27",
        }
        params = (filters, settings)
        return self._magic(params)
