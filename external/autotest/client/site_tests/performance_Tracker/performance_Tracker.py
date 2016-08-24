# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import csv
import logging
import os
import time

from autotest_lib.client.bin import site_utils
from autotest_lib.client.bin import test
from autotest_lib.client.bin import utils

# Measurement duration [seconds] for one interation.
MEASUREMENT_DURATION = 10

TERMINATE_PATH = "/tmp/terminate"

# Time for initial test setup [seconds].
STABILIZATION_DURATION = 60

PERF_RESULT_FILE = '/tmp/perf.csv'

class performance_Tracker(test.test):
    """Monitors cpu/memory usage."""

    version = 1

    def get_cpu_usage(self):
        """Computes current cpu usage in percentage.

        @returns percentage cpu used as a float.

        """
        cpu_usage_start = site_utils.get_cpu_usage()
        time.sleep(MEASUREMENT_DURATION)
        cpu_usage_end = site_utils.get_cpu_usage()
        return site_utils.compute_active_cpu_time(cpu_usage_start,
                                                      cpu_usage_end) * 100


    def used_mem(self):
        """Computes used memory in percentage.

        @returns percentage memory used as a float.

        """
        total_memory = site_utils.get_mem_total()
        return (total_memory - site_utils.get_mem_free()) * 100 / total_memory


    def run_once(self):
        if os.path.isfile(TERMINATE_PATH):
            os.remove(TERMINATE_PATH)

        time.sleep(STABILIZATION_DURATION)
        perf_keyval = {}
        perf_file = open(PERF_RESULT_FILE, 'w')
        writer = csv.writer(perf_file)
        writer.writerow(['cpu', 'memory'])
        while True:
            # This test runs forever until the terminate file is created.
            if os.path.isfile(TERMINATE_PATH):
                logging.info('Exit flag detected; exiting.')
                perf_file.close()
                return
            perf_keyval['cpu_usage'] = self.get_cpu_usage()
            perf_keyval['memory_usage'] = self.used_mem()
            writer.writerow([perf_keyval['cpu_usage'],
                            perf_keyval['memory_usage']])
            self.write_perf_keyval(perf_keyval)
            time.sleep(MEASUREMENT_DURATION)
        perf_file.close()


    def cleanup(self):
        # cleanup() is run by common_lib/test.py.
        if os.path.isfile(TERMINATE_PATH):
            os.remove(TERMINATE_PATH)
