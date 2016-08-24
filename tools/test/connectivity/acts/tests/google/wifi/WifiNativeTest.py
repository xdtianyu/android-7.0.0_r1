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

import acts.base_test as base_test

class WifiNativeTest(base_test.BaseTestClass):
    tests = None

    def __init__(self, controllers):
        base_test.BaseTestClass.__init__(self, controllers)
        self.device = self.native_android_devices[0]
        self.tests = (
                "test_hal_get_features",
        )

    def setup_test(self):
#        TODO: uncomment once wifi_toggle_state (or alternative)
#              work with sl4n
#        assert wutils.wifi_toggle_state(self.device, True)
        return self.device.droid.WifiInit()

#   TODO: uncomment once wifi_toggle_state (or alternative)
#         work with sl4n
#    def teardown_class(self):
#        assert wutils.wifi_toggle_state(self.device, False)

    def test_hal_get_features(self):
        result = self.device.droid.WifiGetSupportedFeatureSet()
        self.log.info("Wi-Fi feature set: {}".format(result))
