# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, time
from autotest_lib.client.bin import test
from autotest_lib.client.cros import power_status


class power_StatsUSB(test.test):
    version = 1


    def run_once(self, test_time=60):
        usb = power_status.USBSuspendStats()

        # get USB percent active since boot
        usb.incremental = False
        stats = usb.refresh()
        logging.info('USB active time since boot: %.2f%%', stats['active'])

        # sleep for some time
        time.sleep(test_time)

        # get USB percent active during the test time
        usb.incremental = True
        stats = usb.refresh()
        logging.info('USB active time in the last %d seconds: %.2f%%',
                     test_time, stats['active'])
