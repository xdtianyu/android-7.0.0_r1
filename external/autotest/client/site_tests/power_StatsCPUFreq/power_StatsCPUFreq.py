# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, time
from autotest_lib.client.bin import test
from autotest_lib.client.cros import power_status


class power_StatsCPUFreq(test.test):
    version = 1


    def run_once(self, test_time=60):
        cpufreq_stats = power_status.CPUFreqStats()

        # log CPU frequency stats since boot
        cpufreq_stats.incremental = False
        current_stats = cpufreq_stats.refresh()
        logging.info('CPUFreq stats since boot:\n %s', current_stats)

        # sleep for some time to allow the system to go into idle state
        time.sleep(test_time)

        # get updated CPU frequency stats
        cpufreq_stats.incremental = True
        current_stats = cpufreq_stats.refresh()
        logging.info('CPUFreq stats in the last %d seconds :\n %s',
                     test_time, current_stats)

