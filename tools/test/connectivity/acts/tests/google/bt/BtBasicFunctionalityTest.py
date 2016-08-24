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
Test script to execute Bluetooth basic functionality test cases.
This test was designed to be run in a shield box.
"""

import time

from queue import Empty
from acts.test_utils.bt.BluetoothBaseTest import BluetoothBaseTest
from acts.test_utils.bt.BtEnum import BluetoothScanModeType
from acts.test_utils.bt.bt_test_utils import check_device_supported_profiles
from acts.test_utils.bt.bt_test_utils import log_energy_info
from acts.test_utils.bt.bt_test_utils import reset_bluetooth
from acts.test_utils.bt.bt_test_utils import set_device_name
from acts.test_utils.bt.bt_test_utils import set_bt_scan_mode
from acts.test_utils.bt.bt_test_utils import setup_multiple_devices_for_bt_test
from acts.test_utils.bt.bt_test_utils import take_btsnoop_logs


class BtBasicFunctionalityTest(BluetoothBaseTest):
    default_timeout = 10
    scan_discovery_time = 5

    def __init__(self, controllers):
        BluetoothBaseTest.__init__(self, controllers)
        self.droid_ad = self.android_devices[0]
        self.droid1_ad = self.android_devices[1]

    def setup_class(self):
        return setup_multiple_devices_for_bt_test(self.android_devices)

    def setup_test(self):
        self.log.debug(log_energy_info(self.android_devices, "Start"))
        for a in self.android_devices:
            a.ed.clear_all_events()
        return True

    def teardown_test(self):
        self.log.debug(log_energy_info(self.android_devices, "End"))
        return True

    def on_fail(self, test_name, begin_time):
        take_btsnoop_logs(self.android_devices, self, test_name)
        reset_bluetooth(self.android_devices)

    @BluetoothBaseTest.bt_test_wrap
    def test_bluetooth_reset(self):
        """Test resetting bluetooth.

        Test the integrity of resetting bluetooth on Android.

        Steps:
        1. Toggle bluetooth off.
        2. Toggle bluetooth on.

        Expected Result:
        Bluetooth should toggle on and off without failing.

        Returns:
          Pass if True
          Fail if False

        TAGS: Classic
        Priority: 1
        """
        return reset_bluetooth([self.droid_ad])

    @BluetoothBaseTest.bt_test_wrap
    def test_make_device_discoverable(self):
        """Test device discoverablity.

        Test that verifies devices is discoverable.

        Steps:
        1. Initialize two android devices
        2. Make device1 discoverable
        3. Check discoverable device1 scan mode
        4. Make device2 start discovery
        5. Use device2 get all discovered bluetooth devices list
        6. Verify device1 is in the list

        Expected Result:
        Device1 is in the discovered devices list.

        Returns:
          Pass if True
          Fail if False

        TAGS: Classic
        Priority: 1
        """
        self.droid_ad.droid.bluetoothMakeDiscoverable()
        scan_mode = self.droid_ad.droid.bluetoothGetScanMode()
        if (scan_mode ==
                BluetoothScanModeType.SCAN_MODE_CONNECTABLE_DISCOVERABLE.value):
            self.log.debug("Android device1 scan mode is "
                           "SCAN_MODE_CONNECTABLE_DISCOVERABLE")
        else:
            self.log.debug("Android device1 scan mode is not "
                           "SCAN_MODE_CONNECTABLE_DISCOVERABLE")
            return False
        if self.droid1_ad.droid.bluetoothStartDiscovery():
            self.log.debug("Android device2 start discovery process success")
            # Give Bluetooth time to discover advertising devices
            time.sleep(self.scan_discovery_time)
            droid_name = self.droid_ad.droid.bluetoothGetLocalName()
            get_all_discovered_devices = self.droid1_ad.droid.bluetoothGetDiscoveredDevices(
            )
            find_flag = False
            if get_all_discovered_devices:
                self.log.debug(
                    "Android device2 get all the discovered devices "
                    "list {}".format(get_all_discovered_devices))
                for i in get_all_discovered_devices:
                    if 'name' in i and i['name'] == droid_name:
                        self.log.debug("Android device1 is in the discovery "
                                       "list of device2")
                        find_flag = True
                        break
            else:
                self.log.debug(
                    "Android device2 get all the discovered devices "
                    "list is empty")
                return False
        else:
            self.log.debug("Android device2 start discovery process error")
            return False
        if not find_flag:
            return False
        return True

    @BluetoothBaseTest.bt_test_wrap
    def test_make_device_undiscoverable(self):
        """Test device un-discoverability.

        Test that verifies device is un-discoverable.

        Steps:
        1. Initialize two android devices
        2. Make device1 un-discoverable
        3. Check un-discoverable device1 scan mode
        4. Make device2 start discovery
        5. Use device2 get all discovered bluetooth devices list
        6. Verify device1 is not in the list

        Expected Result:
        Device1 should not be in the discovered devices list.

        Returns:
          Pass if True
          Fail if False

        TAGS: Classic
        Priority: 1
        """
        self.droid_ad.droid.bluetoothMakeUndiscoverable()
        set_bt_scan_mode(self.droid1_ad,
                         BluetoothScanModeType.SCAN_MODE_NONE.value)
        scan_mode = self.droid1_ad.droid.bluetoothGetScanMode()
        if scan_mode == BluetoothScanModeType.SCAN_MODE_NONE.value:
            self.log.debug("Android device1 scan mode is SCAN_MODE_NONE")
        else:
            self.log.debug("Android device1 scan mode is not SCAN_MODE_NONE")
            return False
        if self.droid1_ad.droid.bluetoothStartDiscovery():
            self.log.debug("Android device2 start discovery process success")
            # Give Bluetooth time to discover advertising devices
            time.sleep(self.scan_discovery_time)
            droid_name = self.droid_ad.droid.bluetoothGetLocalName()
            get_all_discovered_devices = self.droid1_ad.droid.bluetoothGetDiscoveredDevices(
            )
            find_flag = False
            if get_all_discovered_devices:
                self.log.debug(
                    "Android device2 get all the discovered devices "
                    "list {}".format(get_all_discovered_devices))
                for i in get_all_discovered_devices:
                    if 'name' in i and i['name'] == droid_name:
                        self.log.debug(
                            "Android device1 is in the discovery list of "
                            "device2")
                        find_flag = True
                        break
            else:
                self.log.debug("Android device2 found no devices.")
                return True
        else:
            self.log.debug("Android device2 start discovery process error")
            return False
        if find_flag:
            return False
        return True

    @BluetoothBaseTest.bt_test_wrap
    def test_set_device_name(self):
        """Test bluetooth device name.

        Test that a single device can be set device name.

        Steps:
        1. Initialize one android devices
        2. Set device name
        3. Return true is set device name success

        Expected Result:
        Bluetooth device name is set correctly.

        Returns:
          Pass if True
          Fail if False

        TAGS: Classic
        Priority: 1
        """
        name = "SetDeviceName"
        return set_device_name(self.droid_ad.droid, name)

    def test_scan_mode_off(self):
        """Test disabling bluetooth scanning.

        Test that changes scan mode to off.

        Steps:
        1. Initialize android device.
        2. Set scan mode STATE_OFF by disabling bluetooth.
        3. Verify scan state.

        Expected Result:
        Verify scan state is off.

        Returns:
          Pass if True
          Fail if False

        TAGS: Classic
        Priority: 1
        """
        self.log.debug("Test scan mode STATE_OFF.")
        return set_bt_scan_mode(self.droid_ad,
                                BluetoothScanModeType.STATE_OFF.value)

    @BluetoothBaseTest.bt_test_wrap
    def test_scan_mode_none(self):
        """Test bluetooth scan mode none.

        Test that changes scan mode to none.

        Steps:
        1. Initialize android device.
        2. Set scan mode SCAN_MODE_NONE by disabling bluetooth.
        3. Verify scan state.

        Expected Result:
        Verify that scan mode is set to none.

        Returns:
          Pass if True
          Fail if False

        TAGS: Classic
        Priority: 1
        """
        self.log.debug("Test scan mode SCAN_MODE_NONE.")
        return set_bt_scan_mode(self.droid_ad,
                                BluetoothScanModeType.SCAN_MODE_NONE.value)

    @BluetoothBaseTest.bt_test_wrap
    def test_scan_mode_connectable(self):
        """Test bluetooth scan mode connectable.

        Test that changes scan mode to connectable.

        Steps:
        1. Initialize android device.
        2. Set scan mode SCAN_MODE_CONNECTABLE.
        3. Verify scan state.

        Expected Result:
        Verify that scan mode is set to connectable.

        Returns:
          Pass if True
          Fail if False

        TAGS: Classic
        Priority: 2
        """
        self.log.debug("Test scan mode SCAN_MODE_CONNECTABLE.")
        return set_bt_scan_mode(
            self.droid_ad, BluetoothScanModeType.SCAN_MODE_CONNECTABLE.value)

    @BluetoothBaseTest.bt_test_wrap
    def test_scan_mode_connectable_discoverable(self):
        """Test bluetooth scan mode connectable.

        Test that changes scan mode to connectable.

        Steps:
        1. Initialize android device.
        2. Set scan mode SCAN_MODE_DISCOVERABLE.
        3. Verify scan state.

        Expected Result:
        Verify that scan mode is set to discoverable.

        Returns:
          Pass if True
          Fail if False

        TAGS: Classic
        Priority: 2
        """
        self.log.debug("Test scan mode SCAN_MODE_CONNECTABLE_DISCOVERABLE.")
        return set_bt_scan_mode(
            self.droid_ad,
            BluetoothScanModeType.SCAN_MODE_CONNECTABLE_DISCOVERABLE.value)

    @BluetoothBaseTest.bt_test_wrap
    def test_if_support_hid_profile(self):
        """ Test that a single device can support HID profile.
        Steps
        1. Initialize one android devices
        2. Check devices support profiles and return a dictionary
        3. Check the value of key 'hid'

        Expected Result:
        Device1 is in the discovered devices list.

        Returns:
          Pass if True
          Fail if False

        TAGS: Classic
        Priority: 1
        """
        profiles = check_device_supported_profiles(self.droid_ad.droid)
        if not profiles['hid']:
            self.log.debug("Android device do not support HID profile.")
            return False
        return True

    @BluetoothBaseTest.bt_test_wrap
    def test_if_support_hsp_profile(self):
        """ Test that a single device can support HSP profile.
        Steps
        1. Initialize one android devices
        2. Check devices support profiles and return a dictionary
        3. Check the value of key 'hsp'
        :return: test_result: bool
        """
        profiles = check_device_supported_profiles(self.droid_ad.droid)
        if not profiles['hsp']:
            self.log.debug("Android device do not support HSP profile.")
            return False
        return True

    @BluetoothBaseTest.bt_test_wrap
    def test_if_support_a2dp_profile(self):
        """ Test that a single device can support A2DP profile.
        Steps
        1. Initialize one android devices
        2. Check devices support profiles and return a dictionary
        3. Check the value of key 'a2dp'
        :return: test_result: bool
        """
        profiles = check_device_supported_profiles(self.droid_ad.droid)
        if not profiles['a2dp']:
            self.log.debug("Android device do not support A2DP profile.")
            return False
        return True

    @BluetoothBaseTest.bt_test_wrap
    def test_if_support_avrcp_profile(self):
        """ Test that a single device can support AVRCP profile.
        Steps
        1. Initialize one android devices
        2. Check devices support profiles and return a dictionary
        3. Check the value of key 'avrcp'
        :return: test_result: bool
        """
        profiles = check_device_supported_profiles(self.droid_ad.droid)
        if not profiles['avrcp']:
            self.log.debug("Android device do not support AVRCP profile.")
            return False
        return True
