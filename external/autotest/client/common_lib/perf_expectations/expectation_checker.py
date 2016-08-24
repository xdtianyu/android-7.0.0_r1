# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Automated performance regression detection tool for ChromeOS perf tests.

   Refer to the instruction on how to use this tool at
   https://sites.google.com/a/chromium.org/dev/perf-regression-detection.
"""

import logging
import os
import re

import common
from autotest_lib.client.common_lib import site_utils


class TraceNotFound(RuntimeError):
    """Catch the error when an expectation is not defined for a trace."""
    pass


def divide(x, y):
    if y == 0:
        return float('inf')
    return float(x) / y


class perf_expectation_checker(object):
    """Check performance results against expectations."""

    def __init__(self, test_name, board=None,
                 expectation_file_path=None):
        """Initialize a perf expectation checker.

           @param test_name: the name of the performance test,
               will be used to load the expectation.
           @param board: an alternative board name, will be used
               to load the expectation. Defaults to the board name
               in /etc/lsb-release.
           @expectation_file_path: an alternative expectation file.
               Defaults to perf_expectations.json under the same folder
               of this file.
        """
        self._expectations = {}
        if expectation_file_path:
            self._expectation_file_path = expectation_file_path
        else:
            self._expectation_file_path = os.path.abspath(
                os.path.join(os.path.dirname(__file__),
                    'perf_expectations.json'))
        self._board = board or site_utils.get_current_board()
        self._test_name = test_name
        assert self._board, 'Failed to get board name.'
        assert self._test_name, (
               'You must specify a test name when initialize'
               ' perf_expectation_checker.')
        self._load_perf_expectations_file()

    def _load_perf_expectations_file(self):
        """Load perf expectation file."""
        try:
            expectation_file = open(self._expectation_file_path)
        except IOError, e:
            logging.error('I/O Error reading expectations %s(%s): %s',
                          self._expectation_file_path, e.errno, e.strerror)
            raise e
        # Must import here to make it work with autotest.
        import json
        try:
            self._expectations = json.load(expectation_file)
        except ValueError, e:
            logging.error('ValueError parsing expectations %s(%s): %s',
                          self._expectation_file_path, e.errno, e.strerror)
            raise e
        finally:
            expectation_file.close()

        if not self._expectations:
            # Will skip checking the perf values against expectations
            # when no expecation is defined.
            logging.info('No expectation data found in %s.',
                         self._expectation_file_path)
            return

    def compare_one_trace(self, trace, trace_perf_value):
        """Compare a performance value of a trace with the expectation.

        @param trace: the name of the trace
        @param trace_perf_value: the performance value of the trace.
        @return a tuple like one of the below
            ('regress', 2.3), ('improve', 3.2), ('accept', None)
            where the float numbers are regress/improve ratios,
            or None if expectation for trace is not defined.
        """
        perf_key = '/'.join([self._board, self._test_name, trace])
        if perf_key not in self._expectations:
            raise TraceNotFound('Expectation for trace %s not defined' % trace)
        perf_data = self._expectations[perf_key]
        regress = float(perf_data['regress'])
        improve = float(perf_data['improve'])
        if (('better' in perf_data and perf_data['better'] == 'lower') or
            ('better' not in perf_data and regress > improve)):
            # The "lower is better" case.
            if trace_perf_value < improve:
                ratio = 1 - divide(trace_perf_value, improve)
                return 'improve', ratio
            elif trace_perf_value > regress:
                ratio = divide(trace_perf_value, regress) - 1
                return 'regress', ratio
        else:
            # The "higher is better" case.
            if trace_perf_value > improve:
                ratio = divide(trace_perf_value, improve) - 1
                return 'improve', ratio
            elif trace_perf_value < regress:
                ratio = 1 - divide(trace_perf_value, regress)
                return 'regress', ratio
        return 'accept', None

    def compare_multiple_traces(self, perf_results):
        """Compare multiple traces with corresponding expectations.

        @param perf_results: a dictionary from trace name to value in float,
            e.g {"milliseconds_NewTabCalendar": 1231.000000
                 "milliseconds_NewTabDocs": 889.000000}.

        @return a dictionary of regressions, improvements, and acceptances
            of the format below:
            {'regress': [('trace_1', 2.35), ('trace_2', 2.83)...],
             'improve': [('trace_3', 2.55), ('trace_3', 52.33)...],
             'accept':  ['trace_4', 'trace_5'...]}
            where the float number is the regress/improve ratio.
        """
        ret_val = {'regress':[], 'improve':[], 'accept':[]}
        for trace in perf_results:
            try:
                # (key, ratio) is like ('regress', 2.83)
                key, ratio = self.compare_one_trace(trace, perf_results[trace])
                ret_val[key].append((trace, ratio))
            except TraceNotFound:
                logging.debug(
                    'Skip checking %s/%s/%s, expectation not defined.',
                    self._board, self._test_name, trace)
        return ret_val
