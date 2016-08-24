# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error, utils
from autotest_lib.client.cros import service_stopper

class hardware_MemoryLatency(test.test):
    """Autotest for measuring memory latency.

    This uses lat_mem_rd with different parameters to measure memory and cache
    latencies.
    """
    version = 1

    def _run_benchmarks(self, warmup, num_iterations, max_size_mb,
                        sample_size_kb, random, stride):
        """Run the benchmark.

        This runs the lat_mem_rd benchmark from lmbench 3.
        Args:
          warmup:  integer amount of time to spend warming up in microseconds.
          num_iterations: integer number of times to run the benchmark on each
            size.
          max_size_mb: integer size in MB and if sample_size_kb isn't
            specified, then we'll use this as the only number to report
          sample_size_kb: a list of integers of specific points where we want
            to sample the latency
          random: a boolean which specifies whether a regular stride is used or
            a fully randomized pointer chase
          stride: power of two size integer for a stride between pointers
        """
        r = {}
        sample_sizes = [ int(max_size_mb) * 1024 ]
        sample_sizes.extend(sample_size_kb)

        random_flag = '-t' if random else ''

        cmd = 'lat_mem_rd %s -W %d -N %d %s %d 2>&1' % (random_flag, warmup,
                                                   num_iterations, max_size_mb,
                                                   stride)
        logging.debug('cmd: %s', cmd)
        out = utils.system_output(cmd)
        logging.debug('Output: %s', out)

        # split the output into lines and multiply the first column by
        # 1024 to get kb, lmbench divides by 1024 but truncates the result
        # so we have to use rounding to get the correct size
        for line in out.splitlines():
            s = line.split()
            if len(s) == 2:
                size = int(round(float(s[0]) * 1024))
                latency = float(s[1])
                if size in sample_sizes:
                    logging.debug('Matched on size %d', size)
                    if latency <= 0:
                        raise error.TestFail('invalid latency %f' % latency)
                    self._results['ns_' + str(size) + 'KB'] = latency
            else:
                logging.debug('Ignoring output line: %s', line)


    def initialize(self):
        """Perform necessary initialization prior to test run.

        Private Attributes:
          _results: dict containing keyvals with latency measurements
          _services: service_stopper.ServiceStopper object
        """
        super(hardware_MemoryLatency, self).initialize()
        self._results = {}
        stop = [ 'ui' ]
        stop.extend(service_stopper.ServiceStopper.POWER_DRAW_SERVICES)
        self._services = service_stopper.ServiceStopper(stop)
        self._services.stop_services()


    def run_once(self, warmup=100, num_iterations=20, max_size_mb='32',
                 sample_size_kb=[ int(2), int(192) ], random=False, stride=512):
        self._run_benchmarks(warmup, num_iterations, max_size_mb,
                             sample_size_kb, random, stride)
        self.write_perf_keyval(self._results)


    def cleanup(self):
        self._services.restore_services()
        super(hardware_MemoryLatency, self).cleanup()

