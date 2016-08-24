# Copyright (c) 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import glob
import logging
import os

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error

class kernel_Delay(test.test):
    """
    Test to ensure that udelay() delays at least as long as requested
    (as compared to ktime()).

    Test a variety of delays at mininmum and maximum cpu frequencies.

    """
    version = 1

    MIN_KERNEL_VER = '3.8'
    MODULE_NAME = 'udelay_test'
    UDELAY_PATH = '/sys/kernel/debug/udelay_test'
    QUIET_GOVERNOR_PATH = '/sys/devices/system/cpu/cpuquiet/current_governor'
    GOVERNOR_GLOB = '/sys/devices/system/cpu/cpu*/cpufreq/scaling_governor'
    SETSPEED_GLOB = '/sys/devices/system/cpu/cpu*/cpufreq/scaling_setspeed'
    CUR_FREQ_GLOB = '/sys/devices/system/cpu/cpu*/cpufreq/cpuinfo_cur_freq'
    CPUFREQ_AVAIL_PATH = (
            '/sys/devices/system/cpu/cpu0/cpufreq/'
            'scaling_available_frequencies')

    # Test a variety of delays
    # 1..200, 200..500 (by 10), 500..2000 (by 100)
    DELAYS = range(1, 200) + range(200, 500, 10) + range(500, 2001, 100)
    ITERATIONS = 100

    _governor_paths = []
    _setspeed_paths = []
    _cur_freq_paths = []

    def _set_file(self, contents, filename):
        """
        Write a string to a file.

        @param contents: the contents to write to the file
        @param filename: the filename to use

        """
        logging.debug('setting %s to %s', filename, contents)
        with open(filename, 'w') as f:
            f.write(contents)


    def _get_file(self, filename):
        """
        Read a string from a file.

        @returns: the contents of the file (string)

        """
        with open(filename, 'r') as f:
            return f.read()


    def _get_freqs(self):
        """
        Get the current CPU frequencies.

        @returns: the CPU frequencies of each CPU (list of int)

        """
        return [int(self._get_file(p)) for p in self._cur_freq_paths]


    def _get_freqs_string(self):
        """
        Get the current CPU frequencies.

        @returns: the CPU frequencies of each CPU (string)

        """
        return ' '.join(str(x) for x in self._get_freqs())


    def _get_governors(self):
        """
        Get the current CPU governors.

        @returns: the CPU governors of each CPU (list of string)

        """
        return [self._get_file(p).rstrip() for p in self._governor_paths]


    def _get_quiet_governor(self):
        """
        Get the current CPU quiet governor.

        @returns: the CPU quiet governor or None if it does not exist (string)

        """
        if os.path.isfile(self.QUIET_GOVERNOR_PATH):
            return self._get_file(self.QUIET_GOVERNOR_PATH).rstrip()
        else:
            return None


    def _reset_freq(self, initial_governors, initial_quiet_governor):
        """
        Unlimit the CPU frequency.

        @param initial_governors: list of initial governors to reset state to
        @param initial_quiet_governor: initial quiet governor to reset state to

        """
        for p, g in zip(self._governor_paths, initial_governors):
            self._set_file(g, p)
        if initial_quiet_governor and os.path.isfile(self.QUIET_GOVERNOR_PATH):
            self._set_file(initial_quiet_governor, self.QUIET_GOVERNOR_PATH)


    def _set_freq(self, freq):
        """
        Set the CPU frequency.

        @param freq: desired CPU frequency

        """
        # Prevent CPUs from going up and down during the test if the option
        # is available.
        if os.path.isfile(self.QUIET_GOVERNOR_PATH):
            logging.info('changing to userspace cpuquiet governor');
            self._set_file('userspace', self.QUIET_GOVERNOR_PATH)

        for p in self._governor_paths:
            self._set_file('userspace', p)
        for p in self._setspeed_paths:
            self._set_file(str(freq), p)
        logging.info(
                'cpu frequencies set to %s with userspace governor',
                self._get_freqs_string())
        self._check_freq(freq)


    def _check_freq(self, freq):
        """
        Check the CPU frequencies are set as requested.

        @param freq: desired CPU frequency

        """
        for p in self._governor_paths:
            governor = self._get_file(p).rstrip()
            if governor != 'userspace':
                raise error.TestFail('governor changed from userspace to %s' % (
                        governor))
        for p in self._setspeed_paths:
            speed = int(self._get_file(p))
            if speed != freq:
                raise error.TestFail('setspeed changed from %s to %s' % (
                        freq, speed))
        freqs = self._get_freqs()
        for f in freqs:
            if f != freq:
                raise error.TestFail('frequency set to %s instead of %s' % (
                        f, freq))


    def _test_udelay(self, usecs):
        """
        Test udelay() for a given amount of time.

        @param usecs: number of usecs to delay for each iteration

        """
        self._set_file('%d %d' % (usecs, self.ITERATIONS), self.UDELAY_PATH)
        with open(self.UDELAY_PATH, 'r') as f:
            for line in f:
                line = line.rstrip()
                logging.info('result: %s', line)
                if 'FAIL' in line:
                    raise error.TestFail('udelay failed: %s' % line)


    def run_once(self):
        kernel_ver = os.uname()[2]
        if utils.compare_versions(kernel_ver, self.MIN_KERNEL_VER) < 0:
            logging.info(
                    'skipping test: old kernel %s (min %s) missing module %s',
                    kernel_ver, self.MIN_KERNEL_VER, self.MODULE_NAME)
            return

        utils.load_module(self.MODULE_NAME)

        self._governor_paths = glob.glob(self.GOVERNOR_GLOB)
        self._setspeed_paths = glob.glob(self.SETSPEED_GLOB)
        self._cur_freq_paths = glob.glob(self.CUR_FREQ_GLOB)
        initial_governors = self._get_governors()
        initial_quiet_governor = self._get_quiet_governor()

        with open(self.CPUFREQ_AVAIL_PATH, 'r') as f:
            available_freqs = [int(x) for x in f.readline().split()]

        max_freq = max(available_freqs)
        min_freq = min(available_freqs)
        logging.info('cpu frequency max %d min %d', max_freq, min_freq)

        freqs = [ min_freq, max_freq ]
        try:
            for freq in freqs:
                self._set_freq(freq)
                for usecs in self.DELAYS:
                    self._test_udelay(usecs)
                self._check_freq(freq)
        finally:
            self._reset_freq(initial_governors, initial_quiet_governor)
            utils.unload_module(self.MODULE_NAME)
