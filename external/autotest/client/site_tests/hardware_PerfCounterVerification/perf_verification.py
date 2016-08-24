#!/usr/bin/python2.7
# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import itertools
import subprocess
import sys

import numpy

import stats_utils


class Error(Exception):
    """Module error class."""


def GatherPerfStats(program, events, multiplier=1000,
                    progress_func=lambda i, j: None):
    """Run perf stat with the given events and given program.

    @param program: path to benchmark binary. It should take one argument
        (number of loop iterations) and produce no output.
    @param events: value to pass to '-e' arg of perf stat.
    @param multiplier: loop multiplier
    @param progress_func: function that tracks progress of running the
        benchmark. takes two arguments for the outer and inner iteration
        numbers.
    @returns: List of dicts.
    """
    facts = []
    for i, j in itertools.product(xrange(10), xrange(5)):
        progress_func(i, j)
        loops = (i+1) * multiplier
        out = subprocess.check_output(
                ('perf', 'stat', '-x', ',',
                 '-e', events,
                 program, '%d' % loops),
                stderr=subprocess.STDOUT)
        unsupported_events = []
        f = {'loops': loops}
        for line in out.splitlines():
            fields = line.split(',')
            count, unit, event = None, None, None
            if len(fields) == 2:
                count, event = fields
            elif len(fields) == 3:
                count, unit, event = fields
            else:
                raise Error('Unable to parse perf stat output')
            if count == '<not supported>':
                unsupported_events.append(event)
            else:
                f[event] = int(count)
        if unsupported_events:
            raise Error('These events are not supported: %s'
                        % unsupported_events)
        facts.append(f)
    progress_func(-1, -1)  # Finished
    return facts


def main():
    """This can be run stand-alone."""
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

    events = ('cycles', 'instructions')
    facts = GatherPerfStats('src/noploop', ','.join(events),
                            multiplier=10*1000*1000, progress_func=_Progress)

    dt = numpy.dtype([('loops', numpy.int)] +
                     [(e, numpy.int) for e in events])
    a = stats_utils.FactsToNumpyArray(facts, dt)
    for y_var in events:
        print y_var
        (slope, intercept), r2 = stats_utils.LinearRegression(
            a['loops'], a[y_var])
        print "slope:", slope
        print "intercept:", intercept
        print "r-squared:", r2


if __name__ == '__main__':
    main()
