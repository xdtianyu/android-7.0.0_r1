# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import perf_measurement
import numpy

from autotest_lib.client.bin import test
from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error

# This event counts cycles when the page miss handler is servicing page walks
# caused by ITLB misses. Raw event codes for x86 microarchitectures can be
# found at Intel Open Source technology center website:
# https://download.01.org/perfmon
RAW_PAGE_WALK_EVENT_CODES = {
    'Broadwell': 'r1085',
    'Haswell': 'r1085',
    'IvyBridge': 'r0485',
    'SandyBridge': 'r0485',
    'Silvermont': 'r0305',
}

class hardware_TLBMissCost(test.test):
    """Calculates cost of one iTLB miss in
    terms of cycles spent on page walking.
    """

    version = 1
    preserve_srcdir = True

    def initialize(self, events=('cycles', 'iTLB-misses')):
        self.job.require_gcc()
        self.events = events

    def setup(self):
        chost = os.getenv('CHOST', '')
        if chost == 'x86_64-cros-linux-gnu':
            os.chdir(self.srcdir)
            utils.make('clean')
            utils.make()

    def warmup(self):
        uarch = utils.get_intel_cpu_uarch()
        if uarch not in RAW_PAGE_WALK_EVENT_CODES:
            raise error.TestNAError('Unsupported microarchitecture.')
        self.pw_event = RAW_PAGE_WALK_EVENT_CODES.get(uarch)

    def run_once(self, program):
        program = os.path.join(self.srcdir, program)
        self.events = self.events + (self.pw_event,)
        self.facts = perf_measurement.GatherPerfStats(program,
                ','.join(self.events))

    def postprocess_iteration(self):
        results = {}
        if ('iTLB-misses' in self.events):
            pw_cycles_per_miss = [x[self.pw_event] * 1.0 / x['iTLB-misses']
                                  for x in self.facts]
            results['pw-cycles-per-miss'] = numpy.average(pw_cycles_per_miss)
        if ('cycles' in self.events):
            pw_cycle_percent = [x[self.pw_event] * 100.0 / x['cycles']
                                for x in self.facts]
            results['pw-cycle-percent'] = numpy.average(pw_cycle_percent)
        self.write_perf_keyval(results)
