# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import glob, logging, re

from autotest_lib.client.bin import test
from autotest_lib.client.bin import utils
from optparse import OptionParser

class platform_LibCBench(test.test):
    version = 1

    iteration_output = []
    GOVERNOR_FILE = '/sys/devices/system/cpu/cpu*/cpufreq/scaling_governor'

    def run_once(self, args=[]):
        parser = OptionParser()
        parser.add_option('-i',
                          '--iterations',
                          dest='iterations',
                          default=3,
                          help='Number of iterations to run.')
        parser.add_option('--path',
                          dest='path',
                          default='/usr/local/libc-bench/libc-bench',
                          help='Path to the libc-bench binary.')

        options, args = parser.parse_args(args)

        last_governor_modes = []
        governor_paths = glob.glob(self.GOVERNOR_FILE)
        for path in governor_paths:
            mode = utils.system_output('cat %s' % path)
            last_governor_modes.append(mode)
            utils.system('sudo bash -c "echo performance > %s"' % path)

        for i in xrange(int(options.iterations)):
            self.iteration_output.append(utils.system_output(options.path))

        for i in xrange(len(governor_paths)):
            utils.system('sudo bash -c "echo %s > %s"' %
                         (last_governor_modes[i], governor_paths[i]))

    def postprocess_iteration(self):
        results = {}

        current_benchmark = None
        # Process the output of the benchmarks.
        # Output for each benchmark looks like the following:
        # b_<benchmark_1>
        #   time: ..., x: ..., y: ..., z: ...
        for output in self.iteration_output:
            for line in output.split('\n'):
                if line.startswith('b_'):
                    current_benchmark = line
                elif line.strip().startswith('time'):
                    time = float(line.strip().split(',')[0].split(' ')[1])
                    assert(current_benchmark is not None)
                    results.setdefault(current_benchmark, []).append(time)

        perf_results = {}
        for benchmark in results:
            average = sum(results[benchmark]) / len(results[benchmark])
            minimum = min(results[benchmark])
            maximum = max(results[benchmark])
            difference = maximum - minimum
            percent_difference = difference / average * 100


            logging.info('%s:\tmin=%s\tmax=%s\tdiff=%s\tavg=%s\tpercent=%s' %
                         (benchmark, minimum, maximum, difference, average,
                          percent_difference))

            key_string = re.sub('[^\w]', '_', benchmark)
            perf_results[key_string] = average


        self.write_perf_keyval(perf_results)
