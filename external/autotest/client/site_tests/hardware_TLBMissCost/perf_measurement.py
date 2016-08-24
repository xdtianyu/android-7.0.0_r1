#!/usr/bin/python2.7
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import subprocess
import numpy

class Error(Exception):
    """Module error class."""

def GatherPerfStats(program, events):
    """Run perf stat with the given events and given program.

    @param program: path to benchmark binary. It should take one argument
        (number of loop iterations) and produce no output.
    @param events: value to pass to '-e' arg of perf stat.
    @returns: List of dicts.
    """
    facts = []
    for i in xrange(0, 10):
        loops = (i + 1) * 10
        out = subprocess.check_output(
                ('perf', 'stat', '-x', ',',
                 '-e', events,
                 program, '%d' % loops),
                stderr=subprocess.STDOUT)
        unsupported_events = []
        f = {}
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
    return facts
