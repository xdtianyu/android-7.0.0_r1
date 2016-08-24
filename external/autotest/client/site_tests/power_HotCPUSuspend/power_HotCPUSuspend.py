# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import hashlib, logging, multiprocessing, os, re, time
from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import sys_power

SUSPEND_BURN_SECONDS = 10
RESUME_BURN_SECONDS = 5
MIN_CPU_USAGE = .95

PROC_STAT_CPU_FIELDS = ['user', 'nice', 'system', 'idle', 'iowait', 'irq',
                        'softirq', 'steal', 'guest', 'guest_nice']
PROC_STAT_CPU_IDLE_FIELDS = ['idle', 'iowait']

SYSFS_CPUQUIET_ENABLE = '/sys/devices/system/cpu/cpuquiet/tegra_cpuquiet/enable'

def cpu_stress():
    sha512_hash = open('/dev/urandom', 'r').read(64)
    while True:
        sha512_hash = hashlib.sha512(sha512_hash).digest()


def get_system_times():
    proc_stat = utils.read_file('/proc/stat')
    for line in proc_stat.split('\n'):
        if line.startswith('cpu '):
            times = line[4:].strip().split(' ')
            times = [int(jiffies) for jiffies in times]
            return dict(zip(PROC_STAT_CPU_FIELDS, times))


def get_avg_cpu_usage(pre_times, post_times):
    diff_times = {}

    for field in PROC_STAT_CPU_FIELDS:
        diff_times[field] = post_times[field] - pre_times[field]

    idle_time = sum(diff_times[field] for field in PROC_STAT_CPU_IDLE_FIELDS)
    total_time = sum(diff_times[field] for field in PROC_STAT_CPU_FIELDS)

    return float(total_time - idle_time) / total_time


def sleep_and_measure_cpu(sleep_seconds):
    pre_times = get_system_times()
    time.sleep(sleep_seconds)
    post_times = get_system_times()

    avg_cpu_usage = get_avg_cpu_usage(pre_times, post_times)
    logging.info('average CPU utilization, last %ds: %s%%',
                 sleep_seconds, avg_cpu_usage * 100.)
    return avg_cpu_usage


class power_HotCPUSuspend(test.test):
    """Suspend the system with 100% CPU usage."""

    version = 1

    def initialize(self):
        # Store the setting if the system has CPUQuiet feature
        if os.path.exists(SYSFS_CPUQUIET_ENABLE):
            self.is_cpuquiet_enabled = utils.read_file(SYSFS_CPUQUIET_ENABLE)
            utils.write_one_line(SYSFS_CPUQUIET_ENABLE, '0')

    def run_once(self):
        # create processs pool with enough workers to spin all CPUs
        cpus = multiprocessing.cpu_count()
        logging.info('found %d cpus', cpus)
        workers = max(16, cpus * 2)
        pool = multiprocessing.Pool(workers)

        try:
            # fill all CPUs with a spinning task
            logging.info('starting %d workers', workers)
            results = [pool.apply_async(cpu_stress) for _ in xrange(workers)]

            # wait for things to settle
            logging.info('spinning for %d seconds', SUSPEND_BURN_SECONDS)
            if sleep_and_measure_cpu(SUSPEND_BURN_SECONDS) < MIN_CPU_USAGE:
                # There should be no idle time accounted while we're spinning.
                raise error.TestError('unexpected CPU idle time while spinning')

            # go to suspend
            sys_power.kernel_suspend(10)

            # keep spinning after userland resumes
            logging.info('spinning for %d more seconds', RESUME_BURN_SECONDS)
            if sleep_and_measure_cpu(RESUME_BURN_SECONDS) < MIN_CPU_USAGE:
                # There should be no idle time accounted while we're spinning.
                raise error.TestError('unexpected CPU idle time after resume')

            # check workers: if computation completed, something is wrong
            for result in results:
                if result.ready():
                    logging.error('worker finished: %s', result.get())
                    raise error.TestError('worker terminated!')

        finally:
            # kill off the workers
            logging.info('killing %d workers', workers)
            pool.terminate()

    def cleanup(self):
        # Restore the original setting if system has CPUQuiet feature
        if os.path.exists(SYSFS_CPUQUIET_ENABLE):
            utils.open_write_close(
                SYSFS_CPUQUIET_ENABLE, self.is_cpuquiet_enabled)
