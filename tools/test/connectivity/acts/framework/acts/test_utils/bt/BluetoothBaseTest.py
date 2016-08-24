#!/usr/bin/env python3.4
#
#   Copyright 2016 - Google
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
"""
    Base Class for Defining Common Bluetooth Test Functionality
"""

import os
import time
from acts import utils
from acts.base_test import BaseTestClass
from acts.controllers import android_device
from acts.test_utils.bt.bt_test_utils import (
    log_energy_info, reset_bluetooth, setup_multiple_devices_for_bt_test,
    take_btsnoop_logs)


class BluetoothBaseTest(BaseTestClass):
    def __init__(self, controllers):
        BaseTestClass.__init__(self, controllers)

    # Use for logging in the test cases to facilitate
    # faster log lookup and reduce ambiguity in logging.
    def bt_test_wrap(fn):
        def _safe_wrap_test_case(self, *args, **kwargs):
            test_id = "{}:{}:{}".format(self.__class__.__name__, fn.__name__,
                                        time.time())
            log_string = "[Test ID] {}".format(test_id)
            self.log.info(log_string)
            return fn(self, *args, **kwargs)

        return _safe_wrap_test_case

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
        self.log.debug(
            "Test {} failed. Gathering bugreport and btsnoop logs".format(
                test_name))
        take_btsnoop_logs(self.android_devices, self, test_name)
        self._take_bug_report(test_name, begin_time)
        for _ in range(5):
            if reset_bluetooth(self.android_devices):
                break
            else:
                self.log.error("Failed to reset Bluetooth... retrying.")
        return

    def _take_bug_report(self, test_name, begin_time):
        if "no_bug_report_on_fail" in self.user_params:
            return

        # magical sleep to ensure the runtime restart or reboot begins
        time.sleep(1)
        for ad in self.android_devices:
            try:
                ad.adb.wait_for_device()
                ad.take_bug_report(test_name, begin_time)
                tombstone_path = os.path.join(ad.log_path, "BugReports",
                        "{},{}".format(begin_time, ad.serial).replace(' ','_'))
                utils.create_dir(tombstone_path)
                ad.adb.pull('/data/tombstones/', tombstone_path)
            except:
                ad.log.error("Failed to take a bug report for {}, {}"
                             .format(ad.serial, test_name))
