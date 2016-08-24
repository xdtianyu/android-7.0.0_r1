# Copyright (c) 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os
import re
from time import sleep

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error

class kernel_Ktime(test.test):
    """
    Test to ensure that ktime and the RTC clock are consistent.

    """
    version = 1

    MIN_KERNEL_VER = '3.8'
    MODULE_NAME = 'udelay_test'
    UDELAY_PATH = '/sys/kernel/debug/udelay_test'
    RTC_PATH = '/sys/class/rtc/rtc0/since_epoch'

    # How many iterations to run the test for, each iteration is usally
    # a second, but might be more if the skew is too large when retrieving
    # the RTC and ktime.
    TEST_DURATION = 250

    # Allowable drift (as a function of elapsed RTC time): 0.01%
    ALLOWABLE_DRIFT = 0.0001

    # Maximum skew between ktime readings when aligning RTC and ktime
    MAX_SKEW = 0.050

    # Diffs to average for the rolling display
    DIFFS_TO_AVERAGE = 30

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


    def _get_rtc(self):
        """
        Get the current RTC time.

        @returns: the current RTC time since epoch (int)

        """
        return int(self._get_file(self.RTC_PATH))


    def _get_ktime(self):
        """
        Get the current ktime.

        @returns: the current ktime (float)

        """
        # Writing a delay of 0 will return info including the current ktime.
        self._set_file('0', self.UDELAY_PATH)
        with open(self.UDELAY_PATH, 'r') as f:
            for line in f:
                line = line.rstrip()
                logging.debug('result: %s', line)
                m = re.search(r'kt=(\d+.\d+)', line)
                if m:
                    return float(m.group(1))
        return 0.0


    def _get_times(self):
        """
        Get the rtc and estimated ktime and max potential error.

        Returns the RTC and a best guess of the ktime when the RTC actually
        ticked over to the current value.  Also returns the maximum potential
        error of how far they are off by.

        RTC ticked in the range of [ktime - max_error, ktime + max_error]

        @returns: list of the current rtc, estimated ktime, max error

        """
        # Times are read k1, r1, k2, r2, k3.  RTC ticks over somewhere between
        # r1 and r2, but since we don't know exactly when that is, the best
        # guess we have is between k1 and k3.
        rtc_older = self._get_rtc()
        ktime_older = self._get_ktime()
        rtc_old = self._get_rtc()
        ktime_old = self._get_ktime()

        # Ensure that this function returns in a reasonable number of
        # iterations.  If excessive skew occurs repeatedly (eg RTC is too
        # slow), abort.
        bad_skew = 0
        while bad_skew < 10:
            rtc = self._get_rtc()
            ktime = self._get_ktime()
            skew = ktime - ktime_older
            if skew > self.MAX_SKEW:
                # Time between successive calls to ktime was too slow to
                # bound the error to a reasonable value.  A few occurrences
                # isn't anything to be concerned about, but if it's happening
                # every second, it's worth investigating and could indicate
                # that the RTC is very slow and MAX_SKEW needs to be increased.
                logging.info((
                    'retrying excessive skew: '
                    'rtc [%d %d %d] ktime [%f %f %f] skew %f'),
                    rtc_older, rtc_old, rtc, ktime_older, ktime_old, ktime,
                    skew)
                bad_skew += 1
            elif rtc != rtc_old:
                if rtc_older != rtc_old or rtc != rtc_old + 1:
                    # This could happen if we took more than one second per
                    # loop and could be changed to a warning if legitimate.
                    raise error.TestFail('rtc progressed from %u to %u to %u' %
                            (rtc_older, rtc_old, rtc))
                return rtc, ktime_older + skew / 2, skew / 2
            rtc_older = rtc_old
            ktime_older = ktime_old
            rtc_old = rtc
            ktime_old = ktime
        raise error.TestFail('could not reach skew %f after %d attempts' % (
                self.MAX_SKEW, bad_skew))


    def run_once(self):
        kernel_ver = os.uname()[2]
        if utils.compare_versions(kernel_ver, self.MIN_KERNEL_VER) < 0:
            logging.info(
                    'skipping test: old kernel %s (min %s) missing module %s',
                    kernel_ver, self.MIN_KERNEL_VER, self.MODULE_NAME)
            return

        utils.load_module(self.MODULE_NAME)

        start_rtc, start_ktime, start_error = self._get_times()
        logging.info(
                'start rtc %d ktime %f error %f',
                start_rtc, start_ktime, start_error)

        recent_diffs = []
        max_diff = 0
        sum_rtc = 0
        sum_diff = 0
        sum_rtc_rtc = 0
        sum_rtc_diff = 0
        sum_diff_diff = 0
        for i in xrange(self.TEST_DURATION):
            # Sleep some amount of time to avoid busy waiting the entire time
            sleep((i % 10) * 0.1)

            current_rtc, current_ktime, current_error = self._get_times()
            elapsed_rtc = current_rtc - start_rtc
            elapsed_ktime = current_ktime - start_ktime
            elapsed_diff = float(elapsed_rtc) - elapsed_ktime

            # Allow for inaccurate ktime off ALLOWABLE_DRIFT from elapsed RTC,
            # and take into account start and current error in times gathering
            max_error = start_error + current_error
            drift_threshold = elapsed_rtc * self.ALLOWABLE_DRIFT + max_error

            # Track rolling average and maximum diff
            recent_diffs.append(elapsed_diff)
            if len(recent_diffs) > self.DIFFS_TO_AVERAGE:
                recent_diffs.pop(0)
            rolling_diff = sum(recent_diffs) / len(recent_diffs)
            if abs(elapsed_diff) > abs(max_diff):
                max_diff = elapsed_diff

            # Track linear regression
            sum_rtc += elapsed_rtc
            sum_diff += elapsed_diff
            sum_rtc_rtc += elapsed_rtc * elapsed_rtc
            sum_rtc_diff += elapsed_rtc * elapsed_diff
            sum_diff_diff += elapsed_diff * elapsed_diff

            logging.info((
                    'current rtc %d ktime %f error %f; elapsed rtc %d '
                    'ktime %f: threshold %f diff %+f rolling %+f'),
                    current_rtc, current_ktime, current_error, elapsed_rtc,
                    elapsed_ktime, drift_threshold, elapsed_diff, rolling_diff)

            if abs(elapsed_diff) > drift_threshold:
                raise error.TestFail((
                        'elapsed rtc %d and ktime %f diff %f '
                        'is greater than threshold %f') %
                        (elapsed_rtc, elapsed_ktime, elapsed_diff,
                        drift_threshold))

        # Dump final statistics
        logging.info('max_diff %f', max_diff)
        mean_rtc = sum_rtc / self.TEST_DURATION
        mean_diff = sum_diff / self.TEST_DURATION
        slope = ((sum_rtc_diff - sum_rtc * mean_diff) /
                (sum_rtc_rtc - sum_rtc * mean_rtc))
        logging.info('drift %.9f', slope)

        utils.unload_module(self.MODULE_NAME)
