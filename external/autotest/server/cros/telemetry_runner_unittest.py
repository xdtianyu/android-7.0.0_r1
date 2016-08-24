#!/usr/bin/python
#
# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unit tests for server/cros/dynamic_suite/telemetry_runner.py."""
import mox
import unittest

import common

from autotest_lib.server.cros import telemetry_runner


class TelemetryResultTest(mox.MoxTestBase):
    """Unit tests for telemetry_runner.TelemetryResult."""

    SAMPLE_RESULT_LINES = (
        'RESULT average_commit_time_by_url: http___www.ebay.com= 8.86528 ms\n'
        'RESULT CodeLoad: CodeLoad= 6343 score (bigger is better)\n'
        'RESULT ai-astar: ai-astar= '
        '[614,527,523,471,530,523,577,625,614,538] ms\n'
        'RESULT graph_name: test_name= {3.14, 0.98} units')

    EXPECTED_PERF_DATA = [
        {'graph': 'average_commit_time_by_url', 'trace': 'http___www.ebay.com',
         'units': 'ms', 'value': 8.86528},
        {'graph': 'CodeLoad', 'trace': 'CodeLoad',
         'units': 'score__bigger_is_better_', 'value': 6343},
        {'graph': 'ai-astar', 'trace': 'ai-astar',
         'units': 'ms', 'value': 554.2},
        {'graph': 'graph_name', 'trace': 'test_name',
         'units': 'units', 'value': 3.14}]


    def testEmptyStdout(self):
        """Test when the test exits with 0 but there is no output."""
        result = telemetry_runner.TelemetryResult()
        result.parse_benchmark_results()
        self.assertEquals(result.status, telemetry_runner.FAILED_STATUS)


    def testOnlyResultLines(self):
        """Test when the stdout is only Result lines."""
        result = telemetry_runner.TelemetryResult(
                exit_code=0, stdout=self.SAMPLE_RESULT_LINES)
        result.parse_benchmark_results()
        self.assertEquals(result.status, telemetry_runner.SUCCESS_STATUS)
        self.assertEquals(self.EXPECTED_PERF_DATA, result.perf_data)


    def testOnlyResultLinesWithWarnings(self):
        """Test when the stderr has Warnings."""
        stdout = self.SAMPLE_RESULT_LINES
        stderr = ('WARNING: Page failed to load http://www.facebook.com\n'
                  'WARNING: Page failed to load http://www.yahoo.com\n')

        result = telemetry_runner.TelemetryResult(exit_code=2, stdout=stdout,
                                                  stderr=stderr)
        result.parse_benchmark_results()
        self.assertEquals(result.status, telemetry_runner.WARNING_STATUS)
        self.assertEquals(self.EXPECTED_PERF_DATA, result.perf_data)


    def testOnlyResultLinesWithWarningsAndTraceback(self):
        """Test when the stderr has Warnings and Traceback."""
        stdout = self.SAMPLE_RESULT_LINES
        stderr = ('WARNING: Page failed to load http://www.facebook.com\n'
                  'WARNING: Page failed to load http://www.yahoo.com\n'
                  'Traceback (most recent call last):\n'
                  'File "../../utils/unittest_suite.py", line 238, in '
                  '<module>\n'
                  'main()')

        result = telemetry_runner.TelemetryResult(exit_code=2, stdout=stdout,
                                                  stderr=stderr)
        result.parse_benchmark_results()
        self.assertEquals(result.status, telemetry_runner.FAILED_STATUS)
        self.assertEquals(self.EXPECTED_PERF_DATA, result.perf_data)


    def testInfoBeforeResultLines(self):
        """Test when there is info before the Result lines."""
        stdout = ('Pages: [http://www.google.com, http://www.facebook.com]\n' +
                  self.SAMPLE_RESULT_LINES)
        stderr = 'WARNING: Page failed to load http://www.facebook.com\n'

        result = telemetry_runner.TelemetryResult(exit_code=1, stdout=stdout,
                                                  stderr=stderr)
        result.parse_benchmark_results()
        self.assertEquals(result.status, telemetry_runner.WARNING_STATUS)
        self.assertEquals(self.EXPECTED_PERF_DATA, result.perf_data)


    def testInfoAfterResultLines(self):
        """Test when there is info after the Result lines."""
        stdout = (self.SAMPLE_RESULT_LINES + '\n'
                  'stderr:WARNING:root:Found (system), but you do not have '
                  'a DISPLAY environment set.\n\n'
                  '04/16 12:51:23.312 DEBUG|telemetry_:0139|')

        result = telemetry_runner.TelemetryResult(exit_code=0, stdout=stdout,
                                                  stderr='')
        result.parse_benchmark_results()
        self.assertEquals(result.status, telemetry_runner.SUCCESS_STATUS)
        self.assertEquals(self.EXPECTED_PERF_DATA, result.perf_data)


    def testInfoBeforeAndAfterResultLines(self):
        """Test when there is info before and after the Result lines."""
        stdout = ('Pages: [http://www.google.com, http://www.facebook.com]\n' +
                  self.SAMPLE_RESULT_LINES + '\n'
                  'stderr:WARNING:root:Found (system), but you do not have '
                  'a DISPLAY environment set.\n\n'
                  '04/16 12:51:23.312 DEBUG|telemetry_:0139|')

        result = telemetry_runner.TelemetryResult(exit_code=0, stdout=stdout,
                                                  stderr='')
        result.parse_benchmark_results()
        self.assertEquals(result.status, telemetry_runner.SUCCESS_STATUS)
        self.assertEquals(self.EXPECTED_PERF_DATA, result.perf_data)


    def testNoResultLines(self):
        """Test when Result lines are missing from stdout."""
        stdout = ('Pages: [http://www.google.com, http://www.facebook.com]\n'
                  'stderr:WARNING:root:Found (system), but you do not have '
                  'a DISPLAY environment set.\n\n'
                  '04/16 12:51:23.312 DEBUG|telemetry_:0139|')

        result = telemetry_runner.TelemetryResult(exit_code=0, stdout=stdout,
                                                  stderr='')
        result.parse_benchmark_results()
        self.assertEquals(result.status, telemetry_runner.SUCCESS_STATUS)
        self.assertEquals([], result.perf_data)


    def testBadCharactersInResultStringComponents(self):
        """Test bad characters are cleaned up in RESULT string components."""
        stdout = (
            'RESULT average_commit_time_by_url!: '
            'http___www.^^ebay.com= 8.86528 ms\n'
            'RESULT CodeLoad*: CodeLoad= 6343 score\n'
            'RESULT ai-astar: ai-astar= '
            '[614,527,523,471,530,523,577,625,614,538] ~~ms\n'
            'RESULT !!graph_name: &&test_name= {3.14, 0.98} units!')
        expected_perf_data = [
            {'graph': 'average_commit_time_by_url_',
             'trace': 'http___www.__ebay.com',
             'units': 'ms', 'value': 8.86528},
            {'graph': 'CodeLoad_', 'trace': 'CodeLoad',
             'units': 'score', 'value': 6343},
            {'graph': 'ai-astar', 'trace': 'ai-astar',
             'units': '__ms', 'value': 554.2},
            {'graph': '__graph_name', 'trace': '__test_name',
             'units': 'units_', 'value': 3.14}]

        result = telemetry_runner.TelemetryResult(exit_code=0, stdout=stdout,
                                                  stderr='')
        result.parse_benchmark_results()
        self.assertEquals(result.status, telemetry_runner.SUCCESS_STATUS)
        self.assertEquals(expected_perf_data, result.perf_data)


    def testCleanupUnitsString(self):
        """Test that special characters in units strings are cleaned up."""
        result = telemetry_runner.TelemetryResult()
        self.assertEquals(result._cleanup_units_string('score/unit'),
                          'score_per_unit')
        self.assertEquals(result._cleanup_units_string('score / unit'),
                          'score__per__unit')
        self.assertEquals(result._cleanup_units_string('%'),
                          'percent')
        self.assertEquals(result._cleanup_units_string('unit%'),
                          'unitpercent')
        self.assertEquals(result._cleanup_units_string('^^un!ts##'),
                          '__un_ts__')
        self.assertEquals(result._cleanup_units_string('^^un!ts##/time %'),
                          '__un_ts___per_time_percent')


if __name__ == '__main__':
    unittest.main()
