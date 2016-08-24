#!/usr/bin/python
#
# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error

class kernel_CpufreqMinMax(test.test):
    """
    Test to ensure cpufreq user min/max limits can be set and reset.

    Test the kernel's ability to change the min and max cpufreq values
    in both directions.  I.e. we must be able to lower the max
    frequency and then raise it back up again, and raise the minimum
    frequency and lower it back again.  The test on the max is to make
    sure that thermal throttling will work correctly, and the test on
    the min is just for symmetry.
    """
    version = 1

    sys_cpufreq_path = '/sys/devices/system/cpu/cpu0/cpufreq/'

    def _test_freq_set(self, freqs, filename):
        """
        Iteratively write frequencies into a file and check the file.

        This is a helper function for testing the min and max
        given an ordered list, it sets each frequency in the list and
        then checks to make sure it really got set, and raises an
        error if it doesn't match.

        @param freqs: a list of frequencies to set
        @param filename: the filename in the cpufreq directory to use
        """
        for freq in freqs:
            logging.info('setting %s to %d' % (filename, freq))
            f = open(self.sys_cpufreq_path + filename, 'w')
            f.write(str(freq))
            f.close()

            f = open(self.sys_cpufreq_path + filename, 'r')
            cur_freq = int(f.readline())
            f.close()

            if (cur_freq != freq):
                logging.info('%s was set to %d instead of %d' %
                             (filename, cur_freq, freq))
                raise error.TestFail('unable to set %s to %d' %
                                     (filename, freq))


    def run_once(self):
        available_freqs = []
        # When the Intel P-state driver is used, the driver implements an
        # internal governer which selects the currect P-state automatically.
        # Any value between cpuinfo_min_freq and cpuinfo_max_freq is allowed
        # for scaling_max_freq and scaling_min_freq. Setting them is
        # equivalent to setting max_perf_pct and min_perf_pct under
        # /sys/devices/system/cpu/intel_pstate/.
        f = open(self.sys_cpufreq_path + 'scaling_driver', 'r')
        if ('intel_pstate\n' == f.read()):
            fmin = open(self.sys_cpufreq_path + 'cpuinfo_min_freq', 'r')
            fmax = open(self.sys_cpufreq_path + 'cpuinfo_max_freq', 'r')
            available_freqs = map(int, [fmin.read(), fmax.read()])
            fmin.close()
            fmax.close()

            # generate a list of frequencies between min and max
            step = (available_freqs[1] - available_freqs[0]) / 4
            if step:
                available_freqs = range(available_freqs[0],
                                        available_freqs[1] + 1, step)
        f.close()

        if not available_freqs:
            f = open(self.sys_cpufreq_path + 'scaling_available_frequencies', 'r')
            available_freqs = sorted(map(int, f.readline().split()))
            f.close()

        # exit if there are not at least two frequencies
        if (len(available_freqs) < 2):
            return

        # get current maximum scaling frequency
        f = open(self.sys_cpufreq_path + 'scaling_max_freq', 'r')
        max_freq = int(f.readline())
        f.close()

        if max_freq < available_freqs[-1]:
            logging.info(
              'Current maximum frequency %d is lower than available maximum %d',
                max_freq, available_freqs[-1])
            # Board is probably thermally throttled
            if not utils.wait_for_cool_machine():
                raise error.TestFail('Could not get cold machine.')

        # set max to 2nd to highest frequency, then the highest
        self._test_freq_set(available_freqs[-2:], 'scaling_max_freq')

        # set to min 2nd to lowest frequency, then the lowest
        self._test_freq_set(reversed(available_freqs[:2]),
                           'scaling_min_freq')
