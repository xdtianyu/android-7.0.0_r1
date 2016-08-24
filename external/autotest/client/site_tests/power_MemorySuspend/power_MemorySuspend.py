# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import sys_power

class power_MemorySuspend(test.test):
    """Suspend the system via memory_suspend_test."""

    version = 1

    def initialize(self):
        utils.system('stop ui', ignore_status=True)


    def run_once(self, num_suspends=1, max_spurious_wakeup_ratio=0.01):
        spurious_wakeup_count = 0
        max_spurious_wakeup = num_suspends * max_spurious_wakeup_ratio

        for _ in range(num_suspends):
            try:
                sys_power.memory_suspend(10)
            except sys_power.SpuriousWakeupError:
                spurious_wakeup_count += 1
                if spurious_wakeup_count > max_spurious_wakeup:
                    raise error.TestFail('Too many SpuriousWakeupError.')

        if spurious_wakeup_count > 0:
            logging.info("Have %d SpuriousWakeupError", spurious_wakeup_count)

        keyval = { 'numSpuriousWakeupError' : spurious_wakeup_count }
        self.write_perf_keyval(keyval)

    def cleanup(self):
        utils.system('start ui')
