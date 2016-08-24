# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.server import test
from autotest_lib.server.cros import telemetry_runner


class telemetry_ScrollingActionTests(test.test):
    """Run the telemetry scrolling action tests."""
    version = 1


    def run_once(self, host=None):
        """Run the telemetry scrolling action tests.

        @param host: host we are running telemetry on.
        """
        telemetry = telemetry_runner.TelemetryRunner(host)
        result = telemetry.run_telemetry_test('ScrollingActionTest')
        logging.debug('Telemetry completed with a status of: %s with output:'
                      ' %s', result.status, result.output)