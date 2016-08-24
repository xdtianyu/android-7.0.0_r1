# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error
from telemetry.testing import run_chromeos_tests


class telemetry_UnitTests(test.test):
    """This is a client side wrapper for the Telemetry unit tests."""
    version = 1


    def run_once(self, browser_type, unit_tests, perf_tests):
        """Runs telemetry/perf unit tests.

        @param browser_type: The string type of browser to use, e.g., 'system'.
        @param unit_tests: list of unit tests to run, [''] is all tests,
                           [] is no tests.
        @param perf_tests: list of perf unit tests to run, [''] is all tests,
                           [] is no tests.
        """
        tests_to_run = []
        tools_dir = '/usr/local/telemetry/src/tools/'
        if unit_tests:
            tests_to_run.append((os.path.join(tools_dir, 'telemetry'),
                                 unit_tests))
        if perf_tests:
            tests_to_run.append((os.path.join(tools_dir, 'perf'),
                                 perf_tests))
        error_str = run_chromeos_tests.RunChromeOSTests(browser_type,
                                                        tests_to_run)
        if error_str:
            raise error.TestFail(error_str)
