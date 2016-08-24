# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
This is a profiler class for the perf profiler in ChromeOS. It differs from
the existing perf profiler in autotset by directly substituting the options
passed to the initialize function into the "perf" command line. It allows one
to specify which perf command to run and thus what type of profile to collect
(e.g. "perf record" or "perf stat"). It also does not produce a perf report
on the client (where there are no debug symbols) but instead copies
the perf.data file back to the server for analysis.
"""

import os, signal, subprocess
from autotest_lib.client.bin import profiler, os_dep
from autotest_lib.client.common_lib import error


class cros_perf(profiler.profiler):
    version = 1

    def initialize(self, options='-e cycles', profile_type='record'):
        # The two supported options for profile_type are 'record' and 'stat'.
        self.options = options
        self.perf_bin = os_dep.command('perf')
        self.profile_type = profile_type


    def start(self, test):
        if self.profile_type == 'record':
            # perf record allows you to specify where to place the output.
            # perf stat does not.
            logfile = os.path.join(test.profdir, 'perf.data')
            self.options += ' -o %s' % logfile

        if self.profile_type == 'stat':
            # Unfortunately we need to give perf stat a command or process even
            # when running in system-wide mode.
            self.options += ' -p 1'

        outfile = os.path.join(test.profdir, 'perf.out')

        cmd = ('exec %s %s -a %s > %s 2>&1' %
               (self.perf_bin, self.profile_type, self.options, outfile))

        self._process = subprocess.Popen(cmd, shell=True,
                                         stderr=subprocess.STDOUT)


    def stop(self, test):
        ret_code = self._process.poll()
        if ret_code is not None:
            raise error.AutotestError('perf terminated early with return code: '
                                      '%d. Please check your logs.' % ret_code)

        self._process.send_signal(signal.SIGINT)
        self._process.wait()
