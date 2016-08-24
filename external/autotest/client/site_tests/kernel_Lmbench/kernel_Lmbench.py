# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


import logging
from autotest_lib.client.bin import test, utils


class kernel_Lmbench(test.test):
    """Run some benchmarks from the lmbench3 suite.

    lmbench is a series of micro benchmarks intended to measure basic operating
    system and hardware system metrics.

    For further details about lmbench refer to:
    http://lmbench.sourceforge.net/man/lmbench.8.html

    This test is copied from from client/tests to avoid depending on make and
    perl. Here we can also tune the individual benchmarks to be more
    deterministic using taskset, nice, etc.

    Example benchmark runs and outputs on a Lumpy device:
    ./lat_pagefault -N 100 -W 10000 /usr/local/zeros 2>&1
    Pagefaults on /usr/local/zeros: 1.5215 microseconds

    ./lat_syscall -N 100 -W 10000 null 2>&1
    Simple syscall: 0.1052 microseconds

    ./lat_syscall -N 100 -W 10000 read /usr/local/zeros 2>&1
    Simple read: 0.2422 microseconds

    ./lat_syscall -N 100 -W 10000 write /usr/local/zeros 2>&1
    Simple write: 0.2036 microseconds

    ./lat_proc -N 100 -W 10000 fork 2>&1
    Process fork+exit: 250.9048 microseconds

    ./lat_proc -N 100 -W 10000 exec 2>&1
    Process fork+execve: 270.8000 microseconds

    ./lat_mmap -N 100 -W 10000 128M /usr/local/zeros 2>&1
    134.217728 1644

    ./lat_mmap -P 2 -W 10000 128M /usr/local/zeros 2>&1
    134.217728 2932

    ./lat_pipe -N 100 -W 10000 2>&1
    Pipe latency: 14.3242 microseconds

    taskset 0x1 nice -20 ./lat_ctx -s 0 -W 10000  8 2>&1
    "size=0k ovr=1.09
    8 1.80
    """

    version = 1

    def _run_benchmarks(self):
        """Run the benchmarks.

        For details and output format refer to individual benchmark man pages:
        http://lmbench.sourceforge.net/man/

        To improve determinism, we sometimes use taskset to pin to a CPU and
        nice.
        """

        benchmarks = [
            ('lat_pagefault',
             'lat_pagefault -N %(N)d -W %(W)d %(fname)s 2>&1'),
            ('lat_syscall_null',
             'lat_syscall -N %(N)d -W %(W)d null 2>&1'),
            ('lat_syscall_read',
             'lat_syscall -N %(N)d -W %(W)d read %(fname)s 2>&1'),
            ('lat_syscall_write',
             'lat_syscall -N %(N)d -W %(W)d write %(fname)s 2>&1'),
            ('lat_proc_fork',
             'lat_proc -N %(N)d -W %(W)d fork 2>&1'),
            ('lat_proc_exec',
             'lat_proc -N %(N)d -W %(W)d exec 2>&1'),
            ('lat_mmap',
             ('lat_mmap -N %(N)d -W %(W)d '
              '%(fsize)dM %(fname)s 2>&1')),
            ('lat_mmap_P2',
             'lat_mmap -P 2 -W %(W)d %(fsize)dM %(fname)s 2>&1'),
            ('lat_pipe',
             'lat_pipe -N %(N)d -W %(W)d 2>&1'),
            ('lat_ctx_s0',
             ('taskset 0x1 nice -20 '
              'lat_ctx -s 0 -W %(W)d  %(procs)d 2>&1'))
        ]

        keyvals = {}

        # Create a file with <fsize> MB of zeros in /usr/local
        cmd = 'dd if=/dev/zero of=%(fname)s bs=1M count=%(fsize)d'
        cmd = cmd % self.lmparams
        utils.system(cmd)

        for (bm, cmd) in benchmarks:
            cmd = cmd % self.lmparams
            logging.info('Running: %s, cmd: %s', bm, cmd)
            out = utils.system_output(cmd)
            logging.info('Output: %s', out)

            # See class doc string for output examples
            lst = out.split()
            idx = -2
            if '_mmap' in bm or '_ctx' in bm:
                idx = -1
            useconds = float(lst[idx])
            keyvals['us_' + bm] = useconds

        self.lmkeyvals.update(keyvals)


    def initialize(self):
        self.job.require_gcc()
        self.lmkeyvals = {}

        # Common parameters for the benchmarks. More details here:
        # http://lmbench.sourceforge.net/man/lmbench.8.html
        # N - number of repetitions
        # P - parallelism
        # W - warmup time in microseconds
        # fname - file to operate on
        # fsize - size of the above file in MB
        # procs - number of processes for context switch benchmark - lat_ctx
        self.lmparams = {
            'N':100,
            'P':2,
            'fname':'/usr/local/zeros',
            'fsize':128,
            'W':10000,
            'procs':8}

        # Write out the params as kevals now to keep them even if test fails
        param_kvals = [('param_%s' % p,v) for (p,v) in self.lmparams.items()]
        self.write_perf_keyval(dict(param_kvals))

    def run_once(self):
        self._run_benchmarks()
        self.write_perf_keyval(self.lmkeyvals)
