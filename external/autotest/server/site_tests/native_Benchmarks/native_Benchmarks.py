# Copyright (c) 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import shutil
import tempfile

from autotest_lib.server import test

from native_Benchmarks_common import CLIENT_TEST_ROOT
from native_Benchmarks_common import run_check

from octane import octane
from vp8 import vp8

# Benchmark suites
suites = {
    'octane': octane,
    'vp8': vp8,
}

class native_Benchmarks(test.test):
    """Build and run native benchmarks"""
    version = 1

    def run_once(self, client, name, args):
        """
        Build benchmark on the invoking machine and run it on client.

        @param client: The autotest host object representing client.
        @param name: The name of benchmark to run.
        """

        # scratch directory on server.
        scratch_srv = tempfile.mkdtemp()
        try:
            # scratch directory on client.
            cmd = 'mkdir -p %s' % CLIENT_TEST_ROOT
            err_msg = 'Unable to create %s' % CLIENT_TEST_ROOT
            run_check(client, cmd, err_msg)
            scratch_cli = CLIENT_TEST_ROOT

            flags = dict(i.split('=') for i in args)
            results = suites[name](scratch_srv, scratch_cli, client, flags).run()
            for r in results:
                self.output_perf_value(**r)
        finally:
            if scratch_srv and os.path.isdir(scratch_srv):
                shutil.rmtree(scratch_srv)
