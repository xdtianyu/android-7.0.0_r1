#!/usr/bin/python
#
# Copyright (c) 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error


class platform_TraceClockMonotonic(test.test):
    """
    This verifies that the kernel supports monotonic clock timestamps for
    ftrace events.  This is the same clock that Chrome will use for
    timestamping its trace events.
    """
    version = 1

    executable = 'ftrace-clock-monotonic'

    TRACE_PATH = '/sys/kernel/debug/tracing/'
    TRACE_CLOCK = TRACE_PATH + 'trace_clock'
    TRACE_FILE = TRACE_PATH + 'trace'
    TRACE_ENABLE = TRACE_PATH + 'tracing_on'

    def _setup_trace(self):
        """
        Verify that the system supports the monotonic trace clock and set up
        the trace system to use it, and clean up any old stuff in the trace
        and enable it.
        """
        with open(self.TRACE_CLOCK, 'r+') as clock:
            content = clock.read()
            if not 'mono' in content:
                raise error.TestFail('Kernel does not support monotonic clock')

            # Set up to use the monotonic clock
            clock.write('mono')

        # clear out the trace
        with open(self.TRACE_FILE, 'w') as trace:
            trace.write('')

        # enable tracing
        with open(self.TRACE_ENABLE, 'w') as enable:
            enable.write('1')

    def setup(self):
        """Cleans and makes ftrace-clock-monotonic.c.

        Prepares environment for tests by removing directory we will extract
        to (if it exists), extracting tarball of tests, and making them.
        """
        os.chdir(self.srcdir)
        utils.make('clean')
        utils.make()

    def process_trace(self):
        """Opens the trace file and processes it.

        Looks for the 3 markers that are written out by the binary.  The binary
        gets a clock timestamp and then writes it out into the trace three times.
        This looks at each entry and the content of the entry, and verifies that
        they are all in chronological order.
        Example trace file without the header:
           <...>-16484 [003] ...1 509651.512676: tracing_mark_write: start: 509651.512651785
           <...>-16484 [003] ...1 509651.512680: tracing_mark_write: middle: 509651.512678312
           <...>-16484 [003] ...1 509651.512682: tracing_mark_write: end: 509651.512680934
        """
        with open(self.TRACE_FILE, 'r') as trace:
            prev_timestamp = 0
            for line in trace:
                if 'tracing_mark_write' not in line:
                    continue

                columns = line.split()
                entry_timestamp = float(columns[3].replace(':',''))
                sample_timestamp = float(columns[6])
                if sample_timestamp > entry_timestamp:
                    raise error.TestFail('sample timestamp after trace marker entry')

                if sample_timestamp < prev_timestamp:
                    raise error.TestFail('sample timestamp before previous timestamp')
                prev_timestamp = entry_timestamp

            if prev_timestamp == 0:
                raise error.TestFail('no valid timestamps seen in trace file')

    def run_once(self):
        self._setup_trace()
        binpath = os.path.join(self.srcdir, self.executable)
        utils.system_output(binpath, retain_output = True)
        self.process_trace()
