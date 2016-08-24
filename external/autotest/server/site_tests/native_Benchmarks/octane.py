# Copyright (c) 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from native_Benchmarks_common import *
from v8 import v8

class octane(object):
    """Build v8 and run octane with it on client"""

    def __init__(self, scratch_srv, scratch_cli, client, args):
        # Instantiating v8 builds the v8 engine.
        self.v8 = v8(scratch_srv, scratch_cli, client, args)
        self.client = client
        self.scratch_cli = scratch_cli

        # download octane to client
        src = '%s/octane.tar.bz2' % SERVER_TEST_ROOT
        dst = '%s/octane.tar.bz2' % scratch_cli
        rcp_check(client, src, dst,
                  'Error occurred while sending octane to client.\n')

        # unpack octane
        cmd = 'tar jxf %s -C %s' % (dst, scratch_cli)
        run_check(client, cmd, 'Error occurred while unpacking octane')

    def run(self):
        """Returns perf_value tuples"""
        # Octane needs to run in PATH_TO/octane.
        wd = '%s/octane' % self.scratch_cli
        cmd = 'cd %s && %s run_all.js' % (wd, self.v8.executable)
        log = run_check(self.client, cmd, "Error occurred while running v8")
        return self.parse(log)

    def parse(self, log):
        """Translate logs into perf_values tuples.
        @param log: the log to parse
        """
        pairs = [line.split(': ') for line in log.splitlines()]
        del pairs[-2]
        pairs[-1][0] = 'Total'
        return [{'description': 'Octane V2',
                 'graph': p[0],
                 'value': p[1],
                 'units': 'score'} for p in pairs]
