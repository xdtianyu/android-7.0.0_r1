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
Basic LE Stress tests.
"""

import concurrent
import pprint
import time

from queue import Empty
from acts.test_utils.bt.BluetoothBaseTest import BluetoothBaseTest
from acts.test_utils.bt.bt_test_utils import generate_ble_advertise_objects
from acts.test_utils.bt.bt_test_utils import generate_ble_scan_objects
from acts.test_utils.bt.bt_test_utils import get_advanced_droid_list
from acts.test_utils.bt.bt_test_utils import reset_bluetooth
from acts.test_utils.bt.bt_test_utils import scan_result


class BleStressTest(BluetoothBaseTest):
    default_timeout = 10

    def __init__(self, controllers):
        BluetoothBaseTest.__init__(self, controllers)
        self.droid_list = get_advanced_droid_list(self.android_devices)
        self.scn_ad = self.android_devices[0]
        self.adv_ad = self.android_devices[1]
        self.tests = ("test_loop_scanning_1000",
                      "test_restart_scan_callback_after_bt_toggle",
                      "test_start_le_scan_while_toggling_bt", )
        if self.droid_list[0]['max_advertisements'] > 0:
            self.tests = self.tests + (
                "test_loop_advertising_100",
                "test_restart_advertise_callback_after_bt_toggle", )
        if self.droid_list[1]['max_advertisements'] >= 4:
            self.tests = self.tests + (
                "test_loop_scanning_100_verify_no_hci_timeout", )

    def bleadvertise_verify_onsuccess_handler(self, event):
        test_result = True
        self.log.debug("Verifying onSuccess event")
        self.log.debug(pprint.pformat(event))
        return test_result

    @BluetoothBaseTest.bt_test_wrap
    def test_loop_scanning_1000(self):
        """Stress start/stop scan instances.

        This test will start and stop scan instances as fast as possible. This
        will guarantee that the scan instances are properly being cleaned up
        when the scan is stopped.

        Steps:
        1. Start a scan instance.
        2. Stop the scan instance.
        3. Repeat steps 1-2 1000 times.

        Expected Result:
        Neither starting or stopping scan instances causes any failures.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Scanning, Stress
        Priority: 1
        """
        test_result = True
        for _ in range(1000):
            filter_list, scan_settings, scan_callback = generate_ble_scan_objects(
                self.scn_ad.droid)
            self.scn_ad.droid.bleStartBleScan(filter_list, scan_settings,
                                              scan_callback)
            self.scn_ad.droid.bleStopBleScan(scan_callback)
        return test_result

    @BluetoothBaseTest.bt_test_wrap
    def test_loop_scanning_100_verify_no_hci_timeout(self):
        """Stress start/stop scan instances variant.

        This test will start and stop scan instances with a one second timeout
        in between each iteration. This testcase was added because the specific
        timing combination caused hci timeouts.

        Steps:
        1. Start a scan instance.
        2. Stop the scan instance.
        3. Sleep for 1 second.
        4. Repeat steps 1-3 100 times.

        Expected Result:
        Neither starting or stopping scan instances causes any failures.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Scanning, Stress
        Priority: 1
        """
        for _ in range(self.droid_list[1]['max_advertisements']):
            adv_callback, adv_data, adv_settings = generate_ble_advertise_objects(
                self.adv_ad.droid)
            self.adv_ad.droid.bleStartBleAdvertising(adv_callback, adv_data,
                                                     adv_settings)
        for _ in range(100):
            filter_list, scan_settings, scan_callback = generate_ble_scan_objects(
                self.scn_ad.droid)
            self.scn_ad.droid.bleStartBleScan(filter_list, scan_settings,
                                              scan_callback)
            self.log.info(self.scn_ad.ed.pop_event(scan_result.format(
                scan_callback)))
            self.scn_ad.droid.bleStopBleScan(scan_callback)
            time.sleep(1)
        return True

    @BluetoothBaseTest.bt_test_wrap
    def test_loop_advertising_100(self):
        """Stress start/stop advertising instances.

        This test will start and stop advertising instances as fast as possible.

        Steps:
        1. Start a advertising instance.
        2. Find that an onSuccess callback is triggered.
        3. Stop the advertising instance.
        4. Repeat steps 1-3 100 times.

        Expected Result:
        Neither starting or stopping advertising instances causes any failures.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Stress
        Priority: 1
        """
        test_result = True
        for _ in range(100):
            advertise_callback, advertise_data, advertise_settings = generate_ble_advertise_objects(
                self.adv_ad.droid)
            self.adv_ad.droid.bleStartBleAdvertising(
                advertise_callback, advertise_data, advertise_settings)
            expected_advertise_event_name = "".join(["BleAdvertise", str(
                advertise_callback), "onSuccess"])
            worker = self.adv_ad.ed.handle_event(
                self.bleadvertise_verify_onsuccess_handler,
                expected_advertise_event_name, ([]), self.default_timeout)
            try:
                self.log.debug(worker.result(self.default_timeout))
            except Empty as error:
                self.log.debug(" ".join(["Test failed with Empty error:", str(
                    error)]))
                test_result = False
            except concurrent.futures._base.TimeoutError as error:
                self.log.debug(" ".join([
                    "Test failed, filtering callback onSuccess never occurred:",
                    str(error)
                ]))
                test_result = False
            self.adv_ad.droid.bleStopBleAdvertising(advertise_callback)
        return test_result

    @BluetoothBaseTest.bt_test_wrap
    def test_restart_advertise_callback_after_bt_toggle(self):
        """Test to reuse an advertise callback.

        This will verify if advertising objects can be reused after a bluetooth
        toggle.

        Steps:
        1. Start a advertising instance.
        2. Find that an onSuccess callback is triggered.
        3. Stop the advertising instance.
        4. Toggle bluetooth off and on.
        5. Start an advertising instance on the same objects used in step 1.
        6. Find that an onSuccess callback is triggered.

        Expected Result:
        Advertisement should start successfully.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Advertising, Stress
        Priority: 1
        """
        test_result = True
        advertise_callback, advertise_data, advertise_settings = generate_ble_advertise_objects(
            self.adv_ad.droid)
        self.adv_ad.droid.bleStartBleAdvertising(
            advertise_callback, advertise_data, advertise_settings)
        expected_advertise_event_name = "".join(["BleAdvertise", str(
            advertise_callback), "onSuccess"])
        worker = self.adv_ad.ed.handle_event(
            self.bleadvertise_verify_onsuccess_handler,
            expected_advertise_event_name, ([]), self.default_timeout)
        try:
            self.log.debug(worker.result(self.default_timeout))
        except Empty as error:
            self.log.debug(" ".join(["Test failed with Empty error:", str(
                error)]))
            test_result = False
        except concurrent.futures._base.TimeoutError as error:
            self.log.debug(" ".join(
                ["Test failed, filtering callback onSuccess never occurred:",
                 str(error)]))
        test_result = reset_bluetooth([self.scn_ad])
        if not test_result:
            return test_result
        time.sleep(5)
        self.adv_ad.droid.bleStartBleAdvertising(
            advertise_callback, advertise_data, advertise_settings)
        worker = self.adv_ad.ed.handle_event(
            self.bleadvertise_verify_onsuccess_handler,
            expected_advertise_event_name, ([]), self.default_timeout)
        try:
            self.log.debug(worker.result(self.default_timeout))
        except Empty as error:
            self.log.debug(" ".join(["Test failed with Empty error:", str(
                error)]))
            test_result = False
        except concurrent.futures._base.TimeoutError as error:
            self.log.debug(" ".join(
                ["Test failed, filtering callback onSuccess never occurred:",
                 str(error)]))
        return test_result

    @BluetoothBaseTest.bt_test_wrap
    def test_restart_scan_callback_after_bt_toggle(self):
        """Test to reuse an scan callback.

        This will verify if scan objects can be reused after a bluetooth
        toggle.

        Steps:
        1. Start a scanning instance.
        3. Stop the scanning instance.
        4. Toggle bluetooth off and on.
        5. Start an scanning instance on the same objects used in step 1.

        Expected Result:
        Scanner should start successfully.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, Scanning, Stress
        Priority: 1
        """
        test_result = True
        filter_list, scan_settings, scan_callback = generate_ble_scan_objects(
            self.scn_ad.droid)
        self.scn_ad.droid.bleStartBleScan(filter_list, scan_settings,
                                          scan_callback)
        reset_bluetooth([self.scn_ad])
        self.scn_ad.droid.bleStartBleScan(filter_list, scan_settings,
                                          scan_callback)
        return test_result
