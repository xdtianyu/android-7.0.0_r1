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
Basic Bluetooth Classic stress tests.
"""

import time
from acts.base_test import BaseTestClass
from acts.test_utils.bt.bt_test_utils import log_energy_info
from acts.test_utils.bt.bt_test_utils import pair_pri_to_sec
from acts.test_utils.bt.bt_test_utils import reset_bluetooth
from acts.test_utils.bt.bt_test_utils import setup_multiple_devices_for_bt_test


class BtStressTest(BaseTestClass):
    default_timeout = 10

    def __init__(self, controllers):
        BaseTestClass.__init__(self, controllers)

    def setup_class(self):
        return setup_multiple_devices_for_bt_test(self.android_devices)

    def setup_test(self):
        return reset_bluetooth(self.android_devices)

    def setup_test(self):
        setup_result = reset_bluetooth(self.android_devices)
        self.log.debug(log_energy_info(self.android_devices, "Start"))
        for a in self.android_devices:
            a.ed.clear_all_events()
        return setup_result

    def teardown_test(self):
        self.log.debug(log_energy_info(self.android_devices, "End"))
        return True

    def test_toggle_bluetooth(self):
        """Stress test toggling bluetooth on and off.

        Test the integrity of toggling bluetooth on and off.

        Steps:
        1. Toggle bluetooth off.
        2. Toggle bluetooth on.
        3. Repeat steps 1 and 2 one-hundred times.

        Expected Result:
        Each iteration of toggling bluetooth on and off should not cause an
        exception.

        Returns:
          Pass if True
          Fail if False

        TAGS: Classic, Stress
        Priority: 1
        """
        test_result = True
        test_result_list = []
        for n in range(100):
            self.log.info("Toggling bluetooth iteration {}.".format(n + 1))
            test_result = reset_bluetooth([self.android_devices[0]])
            test_result_list.append(test_result)
            if not test_result:
                self.log.debug("Failure to reset Bluetooth... continuing")
        self.log.info("Toggling Bluetooth failed {}/100 times".format(len(
            test_result_list)))
        if False in test_result_list:
            return False
        return test_result

    def test_pair_bluetooth_stress(self):
        """Stress test for pairing BT devices.

        Test the integrity of Bluetooth pairing.

        Steps:
        1. Pair two Android devices
        2. Verify both devices are paired
        3. Unpair devices.
        4. Verify devices unpaired.
        5. Repeat steps 1-4 100 times.

        Expected Result:
        Each iteration of toggling Bluetooth pairing and unpairing
        should succeed.

        Returns:
          Pass if True
          Fail if False

        TAGS: Classic, Stress
        Priority: 1
        """
        for n in range(100):
            self.log.info("Pair bluetooth iteration {}.".format(n + 1))
            if (pair_pri_to_sec(self.android_devices[0].droid,
                                self.android_devices[1].droid) == False):
                self.log.error("Failed to bond devices.")
                return False
            for ad in self.android_devices:
                bonded_devices = ad.droid.bluetoothGetBondedDevices()
                for b in bonded_devices:
                    ad.droid.bluetoothUnbond(b['address'])
                #Necessary sleep time for entries to update unbonded state
                time.sleep(1)
                bonded_devices = ad.droid.bluetoothGetBondedDevices()
                if len(bonded_devices) > 0:
                    self.log.error("Failed to unbond devices: {}".format(
                        bonded_devices))
                    return False
        return True
