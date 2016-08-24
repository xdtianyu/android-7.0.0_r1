# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, numpy, random, time

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import power_suspend, sys_power


class power_SuspendStress(test.test):
    version = 1

    def initialize(self, duration, idle=False, init_delay=0, min_suspend=0,
                   min_resume=0, interface=None):
        """
        Entry point.

        @param duration: total run time of the test
        @param idle: use sys_power.idle_suspend method.
                (use with dummy_IdleSuspend)
        @param init_delay: wait this many seconds before starting the test to
                give parallel tests time to get started
        @param min_suspend: suspend durations will be chosen randomly out of
                the interval between min_suspend and min_suspend + 3 seconds.
        @param min_resume: minimal time in seconds between suspends.
        @param interface: network interface used to connect to the server. If
                specified, will reboot the DUT if the interface is not coming
                back after suspend.
        """
        self._duration = duration
        self._init_delay = init_delay
        self._min_suspend = min_suspend
        self._min_resume = min_resume
        self._interface = interface
        self._method = sys_power.idle_suspend if idle else sys_power.do_suspend


    def run_once(self):
        time.sleep(self._init_delay)
        self._suspender = power_suspend.Suspender(
                self.resultsdir, method=self._method)
        timeout = time.time() + self._duration
        while time.time() < timeout:
            time.sleep(self._min_resume + random.randint(0, 3))
            # Check the network interface to the caller is still available
            if self._interface:
                link_status = None
                try:
                    with open('/sys/class/net/' + self._interface +
                              '/operstate') as link_file:
                        link_status = link_file.readline().strip()
                except:
                    pass
                if link_status != 'up':
                    logging.error('Link to the server gone, reboot')
                    utils.system('reboot')

            self._suspender.suspend(random.randint(0, 3) + self._min_suspend)


    def postprocess_iteration(self):
        if self._suspender.successes:
            keyvals = {'suspend_iterations': len(self._suspender.successes)}
            for key in self._suspender.successes[0]:
                values = [result[key] for result in self._suspender.successes]
                keyvals[key + '_mean'] = numpy.mean(values)
                keyvals[key + '_stddev'] = numpy.std(values)
                keyvals[key + '_min'] = numpy.amin(values)
                keyvals[key + '_max'] = numpy.amax(values)
            self.write_perf_keyval(keyvals)
        if self._suspender.failures:
            total = len(self._suspender.failures)
            iterations = len(self._suspender.successes) + total
            timeout = kernel = firmware = spurious = 0
            for failure in self._suspender.failures:
                if type(failure) is sys_power.SuspendTimeout: timeout += 1
                if type(failure) is sys_power.KernelError: kernel += 1
                if type(failure) is sys_power.FirmwareError: firmware += 1
                if type(failure) is sys_power.SpuriousWakeupError: spurious += 1
            if total == kernel + timeout:
                raise error.TestWarn('%d non-fatal suspend failures in %d '
                        'iterations (%d timeouts, %d kernel warnings)' %
                        (total, iterations, timeout, kernel))
            if total == 1:
                # just throw it as is, makes aggregation on dashboards easier
                raise self._suspender.failures[0]
            raise error.TestFail('%d suspend failures in %d iterations (%d '
                    'timeouts, %d kernel warnings, %d firmware errors, %d '
                    'spurious wakeups)' %
                    (total, iterations, timeout, kernel, firmware, spurious))


    def cleanup(self):
        """
        Clean this up before we wait ages for all the log copying to finish...
        """
        self._suspender.finalize()
