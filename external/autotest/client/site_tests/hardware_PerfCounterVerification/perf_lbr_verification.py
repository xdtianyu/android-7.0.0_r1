#!/usr/bin/python2.7
# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import contextlib
import itertools
import os
import os.path
import re
import subprocess

import numpy

import stats_utils


class Error(Exception):
    """Module error class."""

class TestFail(Error):
    """Indicates a test condition failed (as opposed to tool failure)."""


@contextlib.contextmanager
def CleanupFile(path):
    """Context manager that deletes path on exit."""
    try:
        yield
    finally:
        os.remove(path)


DEVNULL = open('/dev/null', 'w')


class Mmap(object):
    """Represents a memory map, and does the (un)mapping arithmetic."""

    def __init__(self, start, length, pgoff):
        self.start = start
        self.length = length
        self.pgoff = pgoff

    def __repr__(self):
        return '[%x(%x) @ %x]' % (self.start, self.length, self.pgoff)

    def Map(self, ip):
        """Turns ip from a virtual mapped address back to a dso address.

        (Frankly I think these are named backwards. This follows the naming
        convention of perf's struct map.)
        """
        # See perf's util/map.h: map__map_ip()
        return (ip + self.pgoff) - self.start

    def Unmap(self, rip):
        """Turns ip from a dso address into a virtual mapped address."""
        # See perf's util/map.h: map__unmap_ip()
        return self.start + (rip - self.pgoff)

    MMAP_LINE_RE = re.compile(
            r'(?P<event_ts>\d+) '
            r'(?P<event_offset>0x[0-9a-fA-F]+|0) '
            r'[[](?P<event_size>0x[0-9a-fA-F]+|0)[]]: '
            r'PERF_RECORD_MMAP '
            r'(?P<pid>-?\d+)/(?P<tid>-?\d+): '
            r'[[]'
                r'(?P<start>0x[0-9a-fA-F]+|0)'
                r'[(](?P<length>0x[0-9a-fA-F]+|0)[)] @ '
                r'(?P<pgoff>0x[0-9a-fA-F]+|0)'
            r'[]]: '
            r'((?P<executable>[rx]) )?'
            r'(?P<filename>.*)')

    @staticmethod
    def GetFromPerfData(perf_data_filename, mmap_filename):
        """Parse perf_data_filename and find how mmap_filename was mapped.

        @param perf_data_filename: perf.data filename.
        @param mmap_filename: Look for this mmap.
        @returns: Mmap object representing the map for mmap_filename.
        """
        result = None
        raw_trace_proc = subprocess.Popen(
                ('perf', 'report', '-D', '-i', perf_data_filename),
                stdout=subprocess.PIPE, stderr=DEVNULL)
        for line in raw_trace_proc.stdout:
            if 'PERF_RECORD_MMAP' not in line:
                continue
            match = Mmap.MMAP_LINE_RE.match(line)
            if not match:
                raise Error('Unexpected format for MMAP record in raw dump:\n' +
                            line)
            if match.group('filename') == mmap_filename:
                args = match.group('start', 'length', 'pgoff')
                result = Mmap(*tuple(int(x, 16) for x in args))
                break
        for line in raw_trace_proc.stdout:
            # Skip rest of output
            pass
        raw_trace_proc.wait()
        return result

RAW_EVENT_CODES = {
    'br_inst_retired.all_branches': 'r4c4',
}

def TranslateEvents(events):
    return [RAW_EVENT_CODES.get(e, e) for e in events]


# This is the right value for SandyBridge, IvyBridge and Haswell, at least.
# See Intel manual vol. 3B, 17.4.8 LBR
# TODO: Consider detecting if 16 is the correct branch buffer length base on the
# uarch. However, all uarchs we run on have a 16-long buffer.
BRANCH_BUFFER_LENGTH = 16

def EstimateExpectedSamples(loops, count):
    """Calculate the number of SAMPLE events expected.

    ie, expect estimate * BRANCH_BUFFER_LENGTH branches to be sampled.

    Incorporates the "observer effect": includes branches caused by returning
    from PMU interrupts.

    Includes one extra sample due to alignment of samples in the series of
    branches. This sample can be expected "most" of the time, but it is not
    incorrect for it to be missing.

    @param loops: the number of noploop branches executed.
    @param count: the event sampling period. ie, a sample should be collected
        every count branches.
    """
    sample_count = 1 # assume program prolog takes one sample

    all_branches = loops
    loop_samples = loops/(count-1)
    while loop_samples >= 1:
        all_branches += loop_samples
        # compounding branches caused by samples caused by samples caused ...
        loop_samples = loop_samples/(count-1)

    sample_count += all_branches / count
    sample_count += 1  # due to alignment
    return sample_count


def _CountRecordedBranches(perf_data_filename, dso_name, branch_addresses):
    """Count the branches recorded in perf_data_filename using perf report.

    Count the total number of branches recorded, and also the count recorded
    at a specific branch.

    @param perf_data_filename: perf data filename
    @param dso_name: dso that the branch specified by branch_addresses
        pertains to.
    @param branch_addresses: pair of (source, target) addresses specifying the
        branch within dso_name to count.
    @returns: pair with the the total branches recorded, and the count for
        the specified branch.
    """
    mmap = Mmap.GetFromPerfData(perf_data_filename, dso_name)
    out = subprocess.check_output(
            ('perf', 'report', '-i', perf_data_filename, '-nv',
             '-s', 'dso_from,symbol_from,dso_to,symbol_to'),
            stderr=DEVNULL)
    total_sampled_branches = 0
    branch_samples = 0
    for line in out.splitlines():
        if not line or line.startswith('#'):
            continue
        record = line.split()
        samples = int(record[1])
        dso_from = record[2]
        raw_from_address = int(record[3], 16)
        dso_to = record[7]
        raw_to_address = int(record[8], 16)

        # including non-loop branches
        total_sampled_branches += samples

        if not (dso_from == dso_to == dso_name):
            continue
        from_address = mmap.Map(raw_from_address)
        to_address = mmap.Map(raw_to_address)
        if (from_address, to_address) == branch_addresses:
            branch_samples += samples  # should only match once.

    return total_sampled_branches, branch_samples


def GatherPerfBranchSamples(noploop, branch_addresses, events, count,
                            progress_func=lambda i, j: None):
    """Run perf record -b with the given events, and noploop program.

    Expects to record the branch specified by branch_addresses.

    @param noploop: Path to noploop binary. It should take one argument (number
        of loop iterations) and produce no output.
    @param branch_addresses: pair of branch (source, target) addresses.
    @param events: Value to pass to '-e' arg of perf stat, which determines when
        the branch buffer is sampled. ':u' will be appended to each event in
        order to sample only userspace branches. Some events may be translated
        to raw event codes if necessary.
    @param count: Event period to sample.
    @returns: List of dicts containing facts about the executions of noploop.
    """
    events = TranslateEvents(events.split(','))
    events = ','.join(e + ':u' for e in events)
    facts = []
    for i, j in itertools.product(xrange(10), xrange(5)):
        progress_func(i, j)
        loops = (i+1) * 10000000  # (i+1) * 10 million
        fact = {'loops': loops}
        perf_data = 'perf.lbr.noploop.%d.%d.data' % (loops, j)
        with CleanupFile(perf_data):
            subprocess.check_call(
                    ('perf', 'record', '-o', perf_data,
                     '-b', '-e', events, '-c', '%d' % count,
                     noploop, '%d' % loops),
                    stderr=DEVNULL)
            noploop_dso_name = os.path.abspath(noploop)
            total_sampled_branches, branch_samples = _CountRecordedBranches(
                    perf_data, noploop_dso_name, branch_addresses)
            fact['branch_count'] = branch_samples

            total_samples = total_sampled_branches / BRANCH_BUFFER_LENGTH
            total_expected_samples = EstimateExpectedSamples(loops, count)
            if not (total_samples == total_expected_samples or
                    total_samples == total_expected_samples - 1):  # alignment
                raise TestFail('Saw the wrong number of samples: '
                               'saw %d, expected %d or %d' %
                               (total_samples,
                                total_expected_samples,
                                total_expected_samples - 1))

            if fact['branch_count'] == 0:
                raise TestFail('No matching branch records found.')
            facts.append(fact)
    progress_func(-1, -1)  # Finished
    return facts


def ReadBranchAddressesFile(filename):
    with open(filename, 'r') as f:
        branch = tuple(int(x, 16) for x in f.read().split())
    return branch


def main():
    """Verify the operation of LBR using a simple noploop program and perf."""
    def _Progress(i, j):
        if i == -1 and j == -1:  # Finished
            print
            return
        if j == 0:
            if i != 0:
                print
            print i, ':',
        print j,
        sys.stdout.flush()
    branch = ReadBranchAddressesFile('src/noploop_branch.txt')
    facts = GatherPerfBranchSamples('src/noploop', branch,
                                    'br_inst_retired.all_branches',
                                    10000,
                                    progress_func=_Progress)
    dt = numpy.dtype([('loops', numpy.int), ('branch_count', numpy.int)])
    a = stats_utils.FactsToNumpyArray(facts, dt)
    (slope, intercept), r2 = stats_utils.LinearRegression(
        a['loops'], a['branch_count'])
    for f in facts:
        print f
    print "slope:", slope
    print "intercept:", intercept
    print "r-squared:", r2

if __name__ == '__main__':
    main()
