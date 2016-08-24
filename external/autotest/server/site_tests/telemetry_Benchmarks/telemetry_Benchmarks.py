# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.server import test
from autotest_lib.server.cros import telemetry_runner


class telemetry_Benchmarks(test.test):
    """Run a telemetry benchmark."""
    version = 1


    def run_once(self, host=None, benchmark=None, args={}):
        """Run a telemetry benchmark.

        @param host: hostname(ip address) to run the telemetry benchmark on.
        @param benchmark: telemetry benchmark test to run.
        """
        local = args.get("local") == "True"
        telemetry = telemetry_runner.TelemetryRunner(host, local)
        telemetry.run_telemetry_benchmark(benchmark, perf_value_writer=self)
