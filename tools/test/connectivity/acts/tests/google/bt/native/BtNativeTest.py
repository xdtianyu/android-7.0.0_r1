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

import time
from acts.base_test import BaseTestClass
from acts.controllers import native_android_device
from acts.test_utils.bt.native_bt_test_utils import setup_native_bluetooth
from acts.test_utils.bt.bt_test_utils import generate_id_by_size


class BtNativeTest(BaseTestClass):
    tests = None

    def __init__(self, controllers):
        BaseTestClass.__init__(self, controllers)
        setup_native_bluetooth(self.native_devices)
        self.droid = self.native_devices[0].droid
        self.tests = ("test_binder_get_name",
                      "test_binder_get_name_invalid_parameter",
                      "test_binder_set_name_get_name",
                      "test_binder_get_address", )
        if len(self.native_devices) > 1:
            self.droid1 = self.native_devices[1].droid
            self.tests = self.tests + ("test_two_devices_set_get_name", )

    def test_binder_get_name(self):
        result = self.droid.BluetoothBinderGetName()
        self.log.info("Bluetooth device name: {}".format(result))
        return True

    def test_binder_get_name_invalid_parameter(self):
        try:
            self.droid.BluetoothBinderGetName("unexpected_parameter")
            return False
        except Exception:
            return True

    def test_binder_set_name_get_name(self):
        test_name = generate_id_by_size(4)
        result = self.droid.BluetoothBinderSetName(test_name)
        if not result:
            return False
        name = self.droid.BluetoothBinderGetName()
        if test_name != name:
            return False
        return True

    def test_binder_get_address(self):
        result = self.droid.BluetoothBinderGetAddress()
        self.log.info("Found BT address: {}".format(result))
        if not result:
            return False
        return True

    def test_two_devices_set_get_name(self):
        test_name = generate_id_by_size(4)
        for n in self.native_devices:
            d = n.droid
            d.BluetoothBinderSetName(test_name)
            name = d.BluetoothBinderGetName()
            if name != test_name:
                return False
        return True

