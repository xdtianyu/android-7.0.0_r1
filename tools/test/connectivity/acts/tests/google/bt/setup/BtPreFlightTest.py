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
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See thea
# License for the specific language governing permissions and limitations under
# the License.
"""
Bluetooth Pre-Flight Test.
"""

from acts.base_test import BaseTestClass
import os
import pprint


class BtPreFlightTest(BaseTestClass):
    def __init__(self, controllers):
        BaseTestClass.__init__(self, controllers)
        self.tests = ("test_setup_logging", )

    def setup_class(self):
        for a in self.android_devices:
            d = a.droid
            serial = d.getBuildSerial()
            self.log.debug("****START: {} DEVICE INFO****".format(serial))
            self.log.debug("BOOTLOADER VERSION {}".format(d.getBuildBootloader(
            )))
            self.log.debug("BUILD HARDWARE {}".format(d.getBuildHardware()))
            self.log.debug("BUILD PRODUCT {}".format(d.getBuildProduct()))
            self.log.debug("*ENVIRONMENT DETAILS*")
            self.log.debug(pprint.pformat(d.environment()))
            self.log.debug("****END: {} DEVICE INFO****".format(serial))
        return True

    def test_setup_logging(self):
        conf_path = "{}/bt_stack.conf".format(os.path.dirname(os.path.realpath(
            __file__)))
        log_level_check = "TRC_BTM=5"
        remount_check = "remount succeeded"
        for ad in self.android_devices:
            remount_result = ad.adb.remount()
            if remount_check not in str(remount_result):
                # Test for devices that have disable_verity as not all do
                try:
                    ad.adb.disable_verity()
                    ad.reboot()
                    remount_result = ad.adb.remount()
                    if remount_check not in str(remount_result):
                        self.abort_all("Unable to remount device")
                except Exception as e:
                    self.abort_all("Exception in BT pre-flight test: {}"
                                   .format(e))
            ad.adb.push("{} /system/etc/bluetooth/bt_stack.conf".format(
                conf_path))
            result = ad.adb.shell("cat /system/etc/bluetooth/bt_stack.conf")
            # Verify that the log levels have been raised
            if log_level_check not in str(result):
                self.abort_all("BT log levels not set")
        return True
