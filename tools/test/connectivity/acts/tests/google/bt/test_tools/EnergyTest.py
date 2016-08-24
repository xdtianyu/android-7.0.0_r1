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
Continuously poll for energy info for a single Android Device
"""

from contextlib import suppress
from queue import Empty
from acts.test_utils.bt.BluetoothBaseTest import BluetoothBaseTest


class EnergyTest(BluetoothBaseTest):
    def __init__(self, controllers):
        BluetoothBaseTest.__init__(self, controllers)
        self.tests = ("test_continuous_energy_report", )

    @BluetoothBaseTest.bt_test_wrap
    def test_continuous_energy_report(self):
        while (True):
            with suppress(Exception):
                self.log.info(self.android_devices[
                    0].droid.bluetoothGetControllerActivityEnergyInfo(1))
        return True
