# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os, subprocess

from autotest_lib.client.bin import test
from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error

def chain_length(line):
    """
    Return the length of a chain in |line|.
    E.g. if line is "... chain: nr:5" return 5
    """
    return int(line.split(':')[2])

class hardware_PerfCallgraphVerification(test.test):
    """
    Verify perf -g output has a complete call chain in user space.
    """
    version = 1
    preserve_srcdir = True

    def initialize(self):
        self.job.require_gcc()

    def setup(self):
        os.chdir(self.srcdir)
        utils.make('clean')
        utils.make('all')

    def report_has_callchain_length_at_least(self, lines, wanted_length):
        # Look through the output of 'perf report' for the following which
        # shows a long enough callchain from the test graph program:
        # ... PERF_RECORD_SAMPLE(IP, 2): 7015/7015: ...
        # ... chain: nr:5
        # .....  0: fffff
        # .....  1: 00007
        # .....  2: 00007
        # .....  3: 00007
        # .....  4: f5ee2
        # ... thread: test.:7015
        # ...... dso: /tmp/graph.o
        found_sample = False
        length = 0
        for line in lines:
            if 'PERF_RECORD_SAMPLE' in line:
                found_sample = True
            if found_sample and 'chain:' in line:
                length = chain_length(line)
                if not length >= wanted_length:
                    found_sample = False
            if (length >= wanted_length and 'dso:' in line and
                'src/graph' in line):
                return True
        return False

    def run_once(self):
        """
        Collect a perf callchain profile and check the detailed perf report.

        """
        # Waiting on ARM/perf support
        if not utils.get_current_kernel_arch().startswith('x86'):
            return
        # These boards are not supported
        unsupported_boards = ['gizmo']
        board = utils.get_board()
        if board in unsupported_boards:
            return

        try:
            graph = os.path.join(self.srcdir, 'graph')
            perf_file_path = os.tempnam()
            perf_record_args = ['perf', 'record', '-e', 'cycles', '-g', '-o',
                                perf_file_path, '--', graph]
            perf_report_args = ['perf', 'report', '-D', '-i', perf_file_path]

            try:
                subprocess.check_output(perf_record_args,
                                        stderr=subprocess.STDOUT)
            except subprocess.CalledProcessError as cmd_error:
                raise error.TestFail("Running command [%s] failed: %s" %
                                     (' '.join(perf_record_args),
                                      cmd_error.output))

            # Make sure the file still exists.
            if not os.path.isfile(perf_file_path):
                raise error.TestFail('Could not find perf output file: ' +
                                     perf_file_path)

            p = subprocess.Popen(perf_report_args, stdout=subprocess.PIPE)
            result = self.report_has_callchain_length_at_least(p.stdout, 3)
            for _ in p.stdout:
                pass
            p.wait()

        finally:
            os.remove(perf_file_path)

        if not result:
            raise error.TestFail('Callchain not found')

