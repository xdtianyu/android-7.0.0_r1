# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
import os, re, subprocess
from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error


class platform_Perf(test.test):
    """
    Gathers perf data and makes sure it is well-formed.
    """
    version = 1


    def run_once(self):
        """
        Collect a perf data profile and check the detailed perf report.
        """
        keyvals = {}
        num_errors = 0

        try:
            # Create temporary file and get its name. Then close it.
            perf_file_path = os.tempnam()

            # Perf command for recording a profile.
            perf_record_args = [ 'perf', 'record', '-a', '-o', perf_file_path,
                                 '--', 'sleep', '2']
            # Perf command for getting a detailed report.
            perf_report_args = [ 'perf', 'report', '-D', '-i', perf_file_path ]

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

            # Get detailed perf data view and extract the line containing the
            # kernel MMAP summary.
            result = None
            p = subprocess.Popen(perf_report_args, stdout=subprocess.PIPE)
            for line in p.stdout:
                if 'PERF_RECORD_MMAP' in line and 'kallsyms' in line:
                    result = line
                    break;

            # Read the rest of output to EOF.
            for _ in p.stdout:
                pass
            p.wait();

        finally:
            # Delete the perf data file.
            try:
                os.remove(perf_file_path)
            except OSError as e:
                if e.errno != errno.ENONENT: raise

        if result is None:
            raise error.TestFail('Could not find kernel mapping in perf '
                                 'report.')
        # Get the kernel mapping values.
        result = result.split(':')[2]
        start, length, pgoff = re.sub(r'[][()@]', ' ', result).strip().split()

        # Write keyvals.
        keyvals = {}
        keyvals['start'] = start
        keyvals['length'] = length
        keyvals['pgoff'] = pgoff
        self.write_perf_keyval(keyvals)

        # Make sure that the kernel mapping values follow an expected pattern,
        #
        # Expect one of two patterns:
        # (1) start == pgoff, e.g.:
        #   start=0x80008200
        #   pgoff=0x80008200
        #   len  =0xfffffff7ff7dff
        # (2) start < pgoff < start + len, e.g.:
        #   start=0x3bc00000
        #   pgoff=0xffffffffbcc00198
        #   len  =0xffffffff843fffff
        start = int(start, 0)
        length = int(length, 0)
        pgoff = int(pgoff, 0)
        if not (start == pgoff or start < pgoff < start + length):
            raise error.TestFail('Improper kernel mapping values!')
