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
This test script exercises different testcases with a lot of ble beacon traffic.

This test script was designed with this setup in mind:
Shield box one: Android Device as DUT. 7x Sprout devices acting as 192 beacons
"""

import threading

from acts.test_utils.bt.BluetoothBaseTest import BluetoothBaseTest
from acts.test_utils.bt.BleEnum import AdvertiseSettingsAdvertiseMode
from acts.test_utils.bt.BleEnum import ScanSettingsScanMode
from acts.test_utils.bt.bt_test_utils import adv_succ
from acts.test_utils.bt.bt_test_utils import batch_scan_result
from acts.test_utils.bt.bt_test_utils import scan_result
from acts.test_utils.bt.bt_test_utils import generate_ble_advertise_objects
from acts.test_utils.bt.bt_test_utils import generate_ble_scan_objects
from acts.test_utils.bt.bt_test_utils import log_energy_info
from acts.test_utils.bt.bt_test_utils import reset_bluetooth
from acts.test_utils.bt.bt_test_utils import setup_multiple_devices_for_bt_test
from acts.test_utils.bt.bt_test_utils import take_btsnoop_logs


class BeaconSwarmTest(BluetoothBaseTest):
    default_timeout = 10
    beacon_swarm_count = 0
    advertising_device_name_list = []
    discovered_mac_address_list = []

    def __init__(self, controllers):
        BluetoothBaseTest.__init__(self, controllers)
        self.scn_ad = self.android_devices[0]

    def setup_test(self):
        self.log.debug(log_energy_info(self.android_devices, "Start"))
        self.discovered_mac_address_list = []
        for a in self.android_devices:
            a.ed.clear_all_events()
        return True

    def teardown_test(self):
        self.log.debug(log_energy_info(self.android_devices, "End"))
        reset_bluetooth([self.android_devices[0]])
        return True

    def setup_class(self):
        if not setup_multiple_devices_for_bt_test(self.android_devices):
            return False
        return self._start_special_advertisements()

    def cleanup_class(self):
        return reset_bluetooth(self.android_devices)

    def on_fail(self, test_name, begin_time):
        take_btsnoop_logs(self.android_devices, self, test_name)
        reset_bluetooth([self.scn_ad])

    def _start_advertisements_thread(self, ad, beacon_count, restart=False):
        d, e = ad.droid, ad.ed
        if restart:
            try:
                reset_bluetooth([ad])
            except Exception:
                self.log.debug("Failed resetting Bluetooth, continuing...")
                return
        try:
            for _ in range(beacon_count):
                d.bleSetAdvertiseDataIncludeDeviceName(True)
                d.bleSetAdvertiseSettingsAdvertiseMode(
                    AdvertiseSettingsAdvertiseMode.ADVERTISE_MODE_LOW_LATENCY.
                    value)
                advertise_callback, advertise_data, advertise_settings = (
                    generate_ble_advertise_objects(d))
                d.bleStartBleAdvertising(advertise_callback, advertise_data,
                                         advertise_settings)
                try:
                    e.pop_event(
                        adv_succ.format(advertise_callback),
                        self.default_timeout)
                    self.beacon_swarm_count += 1
                    local_bt_name = d.bluetoothGetLocalName()
                    if local_bt_name not in self.advertising_device_name_list:
                        self.advertising_device_name_list.append(
                            d.bluetoothGetLocalName())
                except Exception as e:
                    self.log.info("Advertising failed due to " + str(e))
                self.log.info("Beacons active: {}".format(
                    self.beacon_swarm_count))
        except Exception:
            self.log.debug(
                "Something went wrong in starting advertisements, continuing.")
        return

    def _start_special_advertisements(self):
        self.log.info("Setting up advertisements.")
        beacon_serials = []
        beacon_count = 0
        try:
            beacon_serials = self.user_params['beacon_devices']
            beacon_count = self.user_params['beacon_count']
        except AttributeError:
            self.log.info(
                "No controllable devices connected to create beacons with."
                " Continuing...")
        threads = []
        for a in self.android_devices:
            d, e = a.droid, a.ed
            serial_no = d.getBuildSerial()
            if serial_no not in beacon_serials:
                continue
            thread = threading.Thread(target=self._start_advertisements_thread,
                                      args=(d, e, beacon_count))
            threads.append(thread)
            thread.start()
        for t in threads:
            t.join()
        if self.beacon_swarm_count < (beacon_count * len(beacon_serials)):
            self.log.error("Not enough beacons advertising: {}".format(
                self.beacon_swarm_count))
            return False
        return True

    def _restart_special_advertisements_thread(self):
        beacon_serials = []
        beacon_count = 0
        try:
            beacon_serials = self.user_params['beacon_devices']
            beacon_count = self.user_params['beacon_count']
        except AttributeError:
            self.log.info("No controllable devices connected to create beacons"
                          " with. Continuing...")
        threads = []
        while True:
            self.log.info("Restarting advertisements.")
            for a in self.android_devices:
                d, e = a.droid, a.ed
                serial_no = d.getBuildSerial()
                if serial_no not in beacon_serials:
                    continue
                thread = threading.Thread(
                    target=self._start_advertisements_thread,
                    args=(d, e, beacon_count, True))
                threads.append(thread)
                thread.start()
            for t in threads:
                t.join()
        return True

    def test_swarm_1000_on_scan_result(self):
        """Test LE scanning in a mass beacon deployment.

        Test finding 1000 LE scan results in a mass beacon deployment.

        Steps:
        1. Assume that mass beacon deployment is setup.
        2. Set LE scanning mode to low latency.
        3. Start LE scan.
        4. Pop scan results off the event dispatcher 1000 times.
        5. Stop LE scanning.

        Expected Result:
        1000 scan results should be found without any exceptions.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Scanning, Beacon
        Priority: 1
        """
        self.scn_ad.droid.bleSetScanSettingsScanMode(
            ScanSettingsScanMode.SCAN_MODE_LOW_LATENCY.value)
        filter_list, scan_settings, scan_callback = generate_ble_scan_objects(
            self.scn_ad.droid)
        self.scn_ad.droid.bleStartBleScan(filter_list, scan_settings,
                                          scan_callback)
        for _ in range(1000000):
            event_info = self.scn_ad.ed.pop_event(
                scan_result.format(scan_callback), self.default_timeout)
            mac_address = event_info['data']['Result']['deviceInfo']['address']
            if mac_address not in self.discovered_mac_address_list:
                self.discovered_mac_address_list.append(mac_address)
                self.log.info("Discovered {} different devices.".format(len(
                    self.discovered_mac_address_list)))
        self.log.debug("Discovered {} different devices.".format(len(
            self.discovered_mac_address_list)))
        self.scn_ad.droid.bleStopBleScan(scan_callback)
        return True

    def test_swarm_10000_on_batch_scan_result(self):
        """Test LE batch scanning in a mass beacon deployment.

        Test finding 10000 LE batch scan results in a mass beacon deployment.

        Steps:
        1. Assume that mass beacon deployment is setup.
        2. Set LE scanning mode to low latency and report delay millis to 1
        second.
        3. Start LE scan.
        4. Pop batch scan results off the event dispatcher 10000 times.
        5. Stop LE scanning.

        Expected Result:
        1000 scan results should be found without any exceptions.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Scanning, Beacon
        Priority: 1
        """
        self.scn_ad.droid.bleSetScanSettingsScanMode(
            ScanSettingsScanMode.SCAN_MODE_LOW_LATENCY.value)
        self.scn_ad.droid.bleSetScanSettingsReportDelayMillis(1000)
        filter_list, scan_settings, scan_callback = generate_ble_scan_objects(
            self.scn_ad.droid)
        self.scn_ad.droid.bleStartBleScan(filter_list, scan_settings,
                                          scan_callback)
        for _ in range(10000):
            event_info = self.scn_ad.ed.pop_event(
                batch_scan_result.format(scan_callback), self.default_timeout)
            for result in event_info['data']['Results']:
                mac_address = result['deviceInfo']['address']
                if mac_address not in self.discovered_mac_address_list:
                    self.discovered_mac_address_list.append(mac_address)
        self.log.info("Discovered {} different devices.".format(len(
            self.discovered_mac_address_list)))
        self.scn_ad.droid.bleStopBleScan(scan_callback)
        return True

    def test_swarm_scan_result_filter_each_device_name(self):
        """Test basic LE scan filtering in a mass beacon deployment.

        Test finding LE scan results in a mass beacon deployment. This
        test specifically tests scan filtering of different device names and
        that each device name is found.

        Steps:
        1. Assume that mass beacon deployment is setup with device names
        advertising.
        2. Set LE scanning mode to low latency.
        3. Filter device name from one of the known advertising device names
        4. Start LE scan.
        5. Pop scan results matching the scan filter.
        6. Stop LE scanning.
        7. Repeat steps 2-6 until all advertising device names are found.

        Expected Result:
        All advertising beacons are found by their device name.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Scanning, Beacon, Filtering
        Priority: 1
        """
        for filter_name in self.advertising_device_name_list:
            self.scn_ad.droid.bleSetScanSettingsScanMode(
                ScanSettingsScanMode.SCAN_MODE_LOW_LATENCY.value)
            filter_list, scan_settings, scan_callback = (
                generate_ble_scan_objects(self.scn_ad.droid))
            try:
                self.scn_ad.droid.bleSetScanFilterDeviceName(filter_name)
                self.scn_ad.droid.bleBuildScanFilter(filter_list)
                self.scn_ad.droid.bleStartBleScan(filter_list, scan_settings,
                                                  scan_callback)
                self.log.debug(self.scn_ad.ed.pop_event(
                    scan_result.format(scan_callback), self.default_timeout))
            except Exception:
                self.log.info("Couldn't find advertiser name {}.".format(
                    filter_name))
                return False
            self.scn_ad.droid.bleStopBleScan(scan_callback)
        return True

    def test_swarm_rotate_addresses(self):
        """Test basic LE scan filtering in a mass beacon deployment.

        Test finding LE scan results in a mass beacon deployment. This test
        rotates the mac address of the advertising devices at a consistent
        interval in order to make the scanning device think there are
        thousands of devices nearby.

        Steps:
        1. Assume that mass beacon deployment is setup with device names
        advertising.
        2. Set LE scanning mode to low latency on 28 scan instances.
        3. Start LE scan on each of the scan instances.
        5. Continuously Pop scan results matching the scan filter.
        6. Rotate mac address of each advertising device sequentially.
        7. 5-6 10,000 times.
        8. Stop LE scanning

        Expected Result:
        The Bluetooth stack doesn't crash.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Scanning, Beacon
        Priority: 1
        """
        scan_callback_list = []
        for _ in range(28):
            self.scn_ad.droid.bleSetScanSettingsScanMode(
                ScanSettingsScanMode.SCAN_MODE_LOW_LATENCY.value)
            filter_list, scan_settings, scan_callback = generate_ble_scan_objects(
                self.scn_ad.droid)
            self.scn_ad.droid.bleStartBleScan(filter_list, scan_settings,
                                              scan_callback)
            scan_callback_list.append(scan_callback)
        thread = threading.Thread(
            target=self._restart_special_advertisements_thread,
            args=())
        thread.start()
        n = 0
        while n < 10000:
            for cb in scan_callback_list:
                event_info = self.scn_ad.ed.pop_event(
                    scan_result.format(cb), self.default_timeout)
                mac_address = event_info['data']['Result']['deviceInfo'][
                    'address']
                if mac_address not in self.discovered_mac_address_list:
                    self.discovered_mac_address_list.append(mac_address)
                self.log.info("Discovered {} different devices.".format(len(
                    self.discovered_mac_address_list)))
                n += 1
        self.scn_ad.droid.bleStopBleScan(scan_callback)
        return True
