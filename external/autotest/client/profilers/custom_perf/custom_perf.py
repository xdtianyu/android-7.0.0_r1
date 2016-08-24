# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


import os, signal, subprocess
from autotest_lib.client.bin import profiler, os_dep
from autotest_lib.client.common_lib import error


class custom_perf(profiler.profiler):
    """
    This is a profiler class for the perf profiler in ChromeOS. It differs from
    cros_perf in that you can completely customize what arguments you send in to
    perf.
    """
    version = 1

    def initialize(self, perf_options=''):
        # The two supported options for profile_type are 'record' and 'stat'.
        self.perf_options = perf_options
        self.perf_bin = os_dep.command('perf')


    def start(self, test):
        outfile = os.path.join(test.profdir, 'perf.out')

        cmd = ('cd %s; exec %s %s > %s 2>&1' %
               (test.profdir, self.perf_bin, self.perf_options, outfile))

        self._process = subprocess.Popen(cmd, shell=True,
                                         stderr=subprocess.STDOUT)


    def stop(self, test):
        ret_code = self._process.poll()
        if ret_code is not None:
            raise error.AutotestError('perf terminated early with return code: '
                                      '%d. Please check your logs.' % ret_code)

        os.killpg(os.getpgid(self._process.pid), signal.SIGINT)
        self._process.wait()
