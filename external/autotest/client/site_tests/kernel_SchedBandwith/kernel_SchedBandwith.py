#!/usr/bin/python
#
# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os
import subprocess
import time

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error

class kernel_SchedBandwith(test.test):
    """Test kernel CFS_BANDWIDTH scheduler mechanism (/sys/fs/cgroup/...)"""
    version = 1
    # A 30 second (default) run should result in most of the time slices being
    # throttled.  Set a conservative lower bound based on having an unknown
    # system load.  Alex commonly yields numbers in the range 311..315, which
    # includes test overhead and signal latency.
    _MIN_SECS = 30

    _CG_DIR = "/sys/fs/cgroup/cpu"
    _CG_CRB_DIR = os.path.join(_CG_DIR, "chrome_renderers", "background")

    def _parse_cpu_stats(self):
        """Parse and return CFS bandwidth statistics.

        From kernel/Documentation/scheduler/sched-bwc.txt

        cpu.stat:
        - nr_periods: Number of enforcement intervals that have elapsed.
        - nr_throttled: Number of times the group has been throttled/limited.
        - throttled_time: The total time duration (in nanoseconds) for which entities
          of the group have been throttled.

        Returns: tuple with nr_periods, nr_throttled, throttled_time.
        """
        nr_periods = None
        nr_throttled = None
        throttled_time = None

        fd = open(os.path.join(self._CG_CRB_DIR, "cpu.stat"))

        for ln in fd.readlines():
            logging.debug(ln)
            (name, val) = ln.split()
            logging.debug("name = %s val = %s", name, val)
            if name == 'nr_periods':
                nr_periods = int(val)
            if name == 'nr_throttled':
                nr_throttled = int(val)
            if name == 'throttled_time':
                throttled_time = int(val)

        fd.close()
        return nr_periods, nr_throttled, throttled_time

    @staticmethod
    def _parse_pid_stats(pid):
        """Parse process id stats to determin CPU utilization.

           from: https://www.kernel.org/doc/Documentation/scheduler/sched-stats.txt

           /proc/<pid>/schedstat
           ----------------
           schedstats also adds a new /proc/<pid>/schedstat file to include some
           of the same information on a per-process level.  There are three
           fields in this file correlating for that process to:
                1) time spent on the cpu
                2) time spent waiting on a runqueue
                3) # of timeslices run on this cpu

        Args:
            pid: integer, process id to gather stats for.

        Returns:
            tuple with total_msecs and idle_msecs
        """
        idle_slices = 0
        total_slices = 0

        fname = "/proc/sys/kernel/sched_cfs_bandwidth_slice_us"
        timeslice_ms = int(utils.read_one_line(fname).strip()) / 1000.

        with open(os.path.join('/proc', str(pid), 'schedstat')) as fd:
            values = list(int(val) for val in fd.readline().strip().split())
            running_slices = values[0] / timeslice_ms
            idle_slices = values[1] / timeslice_ms
            total_slices = running_slices + idle_slices
        return (total_slices, idle_slices)


    def _cg_start_task(self, in_cgroup=True):
        """Start a CPU hogging task and add to cgroup.

        Args:
            in_cgroup: Boolean, if true add to cgroup otherwise just start.

        Returns:
            integer of pid of task started
        """
        null_fd = open("/dev/null", "w")
        cmd = ['seq', '0', '0', '0']
        task = subprocess.Popen(cmd, stdout=null_fd)
        self._tasks.append(task)

        if in_cgroup:
            utils.write_one_line(os.path.join(self._CG_CRB_DIR, "tasks"),
                                 task.pid)
        return task.pid


    def _cg_stop_tasks(self):
        """Stop CPU hogging task."""
        if hasattr(self, '_tasks') and self._tasks:
            for task in self._tasks:
                task.kill()
        self._tasks = []


    def _cg_set_quota(self, quota=-1):
        """Set CPU quota that can be used for cgroup

        Default of -1 will disable throttling
        """
        utils.write_one_line(os.path.join(self._CG_CRB_DIR, "cpu.cfs_quota_us"),
                             quota)
        rd_quota = utils.read_one_line(os.path.join(self._CG_CRB_DIR,
                                                    "cpu.cfs_quota_us"))
        if rd_quota != quota:
            error.TestFail("Setting cpu quota to %d" % quota)


    def _cg_total_shares(self):
        if not hasattr(self, '_total_shares'):
            self._total_shares = int(utils.read_one_line(
                    os.path.join(self._CG_DIR, "cpu.shares")))
        return self._total_shares


    def _cg_set_shares(self, shares=None):
        """Set CPU shares that can be used for cgroup

        Default of None reads total shares for cpu group and assigns that so
        there will be no throttling
        """
        if shares is None:
            shares = self._cg_total_shares()
        utils.write_one_line(os.path.join(self._CG_CRB_DIR, "cpu.shares"),
                             shares)
        rd_shares = utils.read_one_line(os.path.join(self._CG_CRB_DIR,
                                                  "cpu.shares"))
        if rd_shares != shares:
            error.TestFail("Setting cpu shares to %d" % shares)


    def _cg_disable_throttling(self):
        self._cg_set_quota()
        self._cg_set_shares()


    def _cg_test_quota(self):
        stats = []
        period_us = int(utils.read_one_line(os.path.join(self._CG_CRB_DIR,
                                                     "cpu.cfs_period_us")))

        stats.append(self._parse_cpu_stats())

        self._cg_start_task()
        self._cg_set_quota(int(period_us * 0.1))
        time.sleep(self._MIN_SECS)

        stats.append(self._parse_cpu_stats())

        self._cg_stop_tasks()
        return stats


    def _cg_test_shares(self):
        stats = []

        self._cg_set_shares(2)
        pid = self._cg_start_task()
        stats.append(self._parse_pid_stats(pid))

        # load system heavily
        for _ in xrange(utils.count_cpus() * 2 + 1):
            self._cg_start_task(in_cgroup=False)

        time.sleep(self._MIN_SECS)

        stats.append(self._parse_pid_stats(pid))

        self._cg_stop_tasks()
        return stats


    @staticmethod
    def _check_stats(name, stats, percent):
        total = stats[1][0] - stats[0][0]
        idle = stats[1][1] - stats[0][1]
        logging.info("%s total:%d idle:%d",
                     name, total, idle)

        # make sure we idled at least X% of the slices
        min_idle = int(percent * total)
        if idle < min_idle:
            logging.error("%s idle count %d < %d ", name, idle,
                          min_idle)
            return 1
        return 0


    def setup(self):
        super(kernel_SchedBandwith, self).setup()
        self._tasks = []
        self._quota = None
        self._shares = None


    def run_once(self, test_quota=True, test_shares=True):
        errors = 0
        if not os.path.exists(self._CG_CRB_DIR):
            raise error.TestError("Locating cgroup dir %s" % self._CG_CRB_DIR)

        self._quota = utils.read_one_line(os.path.join(self._CG_CRB_DIR,
                                                       "cpu.cfs_quota_us"))
        self._shares = utils.read_one_line(os.path.join(self._CG_CRB_DIR,
                                                        "cpu.shares"))
        if test_quota:
            self._cg_disable_throttling()
            quota_stats = self._cg_test_quota()
            errors += self._check_stats('quota', quota_stats, 0.9)

        if test_shares:
            self._cg_disable_throttling()
            shares_stats = self._cg_test_shares()
            errors += self._check_stats('shares', shares_stats, 0.6)

        if errors:
            error.TestFail("Cgroup bandwidth throttling not working")


    def cleanup(self):
        super(kernel_SchedBandwith, self).cleanup()
        self._cg_stop_tasks()

        if hasattr(self, '_quota') and self._quota is not None:
            self._cg_set_quota(self._quota)

        if hasattr(self, '_shares') and self._shares is not None:
            self._cg_set_shares(self._shares)
