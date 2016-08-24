#!/usr/bin/python
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Report summarizer of internal test pass% from running many tests in LTP.

LTP is the Linux Test Project from http://ltp.sourceforge.net/.

This script serves to summarize the results of a test run by LTP test
infrastructure.  LTP frequently runs >1000 tests so summarizing the results
by result-type and count is useful. This script is invoked by the ltp.py
wrapper in Autotest as a post-processing step to summarize the LTP run results
in the Autotest log file.

This script may be invoked by the command-line as follows:

$ ./parse_ltp_out.py -l /mypath/ltp.out
"""

import optparse
import os
import re
import sys


SUMMARY_BORDER = 80 * '-'
# Prefix char used in summaries:
# +: sums into 'passing'
# -: sums into 'notpassing'
TEST_FILTERS = {'TPASS': '+Pass', 'TFAIL': '-Fail', 'TBROK': '-Broken',
                'TCONF': '-Config error', 'TRETR': 'Retired',
                'TWARN': '+Warning'}


def parse_args(argv):
    """Setup command line parsing options.

    @param argv: command-line arguments.

    @return parsed option result from optparse.
    """
    parser = optparse.OptionParser('Usage: %prog --ltp-out-file=/path/ltp.out')
    parser.add_option(
        '-l', '--ltp-out-file',
        help='[required] Path and file name for ltp.out [default: %default]',
        dest='ltp_out_file',
        default=None)
    parser.add_option(
        '-t', '--timings',
        help='Show test timings in buckets [default: %default]',
        dest='test_timings', action='store_true',
        default=False)
    options, args = parser.parse_args()
    if not options.ltp_out_file:
        parser.error('You must supply a value for --ltp-out-file.')

    return options


def _filter_and_count(ltp_out_file, test_filters):
    """Utility function to count lines that match certain filters.

    @param ltp_out_file: human-readable output file from LTP -p (ltp.out).
    @param test_filters: dict of the tags to match and corresponding print tags.

    @return a dictionary with counts of the lines that matched each tag.
    """
    marker_line = '^<<<%s>>>$'
    status_line_re = re.compile('^\w+ +\d+ +(\w+) +: +\w+')
    filter_accumulator = dict.fromkeys(test_filters.keys(), 0)
    parse_states = (
        {'filters': {},
         'terminator': re.compile(marker_line % 'test_output')},
        {'filters': filter_accumulator,
         'terminator': re.compile(marker_line % 'execution_status')})

    # Simple 2-state state machine.
    state_test_active = False
    with open(ltp_out_file) as f:
        for line in f:
            state_index = int(state_test_active)
            if re.match(parse_states[state_index]['terminator'], line):
                # This state is terminated - proceed to next.
                state_test_active = not state_test_active
            else:
                # Determine if this line matches any of the sought tags.
                m = re.match(status_line_re, line)
                if m and m.group(1) in parse_states[state_index]['filters']:
                    parse_states[state_index]['filters'][m.group(1)] += 1
    return filter_accumulator


def _print_summary(filters, accumulator):
    """Utility function to print the summary of the parsing of ltp.out.

    Prints a count of each type of test result, then a %pass-rate score.

    @param filters: map of tags sought and corresponding print headers.
    @param accumulator: counts of test results with same keys as filters.
    """
    print SUMMARY_BORDER
    print 'Linux Test Project (LTP) Run Summary:'
    print SUMMARY_BORDER
    # Size the header to the largest printable tag.
    fmt = '%%%ss: %%s' % max(map(lambda x: len(x), filters.values()))
    for k in sorted(filters):
         print fmt % (filters[k], accumulator[k])

    print SUMMARY_BORDER
    # These calculations from ltprun-summary.sh script.
    pass_count = sum([accumulator[k] for k in filters if filters[k][0] == '+'])
    notpass_count = sum([accumulator[k] for k in filters
                                        if filters[k][0] == '-'])
    total_count = pass_count + notpass_count
    if total_count:
      score = float(pass_count) / float(total_count) * 100.0
    else:
      score = 0.0
    print 'SCORE.ltp: %.2f' % score
    print SUMMARY_BORDER


def _filter_times(ltp_out_file):
    """Utility function to count lines that match certain filters.

    @param ltp_out_file: human-readable output file from LTP -p (ltp.out).

    @return a dictionary with test tags and corresponding times.
            The dictionary is a set of buckets of tests based on the test
            duration:
            0: [tests that recoreded 0sec runtimes],
            1: [tests that recorded runtimes from 0-60sec], ...
            2: [tests that recorded runtimes from 61-120sec], ...
    """
    test_tag_line_re = re.compile('^tag=(\w+)\s+stime=(\d+)$')
    test_duration_line_re = re.compile('^duration=(\d+)\s+.*')
    filter_accumulator = {}
    with open(ltp_out_file) as f:
        previous_tag = None
        previous_time_s = 0
        recorded_tags = set()
        for line in f:
            tag_matches = re.match(test_tag_line_re, line)
            if tag_matches:
                current_tag = tag_matches.group(1)
                if previous_tag:
                    if previous_tag in recorded_tags:
                        print 'WARNING: duplicate tag found: %s.' % previous_tag
                previous_tag = current_tag
                continue
            duration_matches = re.match(test_duration_line_re, line)
            if duration_matches:
                duration = int(duration_matches.group(1))
                if not previous_tag:
                    print 'WARNING: duration without a tag: %s.' % duration
                    continue
                if duration != 0:
                    duration = int(duration / 60) + 1
                test_list = filter_accumulator.setdefault(duration, [])
                test_list.append(previous_tag)
    return filter_accumulator


def _print_timings(accumulator):
    """Utility function to print the summary of the parsing of ltp.out.

    Prints a count of each type of test result, then a %pass-rate score.

    Args:
    @param accumulator: counts of test results
    """
    print SUMMARY_BORDER
    print 'Linux Test Project (LTP) Timing Summary:'
    print SUMMARY_BORDER
    for test_limit in sorted(accumulator.keys()):
        print '<=%smin: %s tags: %s' % (
            test_limit, len(accumulator[test_limit]),
            ', '.join(sorted(accumulator[test_limit])))
        print ''
    print SUMMARY_BORDER
    return


def summarize(ltp_out_file, test_timings=None):
    """Scan detailed output from LTP run for summary test status reporting.

    Looks for all possible test result types know to LTP: pass, fail, broken,
    config error, retired and warning.  Prints a summary.

    @param ltp_out_file: human-readable output file from LTP -p (ltp.out).
    @param test_timings: if True, emit an ordered summary of run timings of
                         tests.
    """
    if not os.path.isfile(ltp_out_file):
        print 'Unable to locate %s.' % ltp_out_file
        return

    _print_summary(TEST_FILTERS, _filter_and_count(ltp_out_file, TEST_FILTERS))
    if test_timings:
      _print_timings(_filter_times(ltp_out_file))


def main(argv):
    """ Parse the human-readable logs from an LTP run and print a summary.

    @param argv: command-line arguments.
    """
    options = parse_args(argv)
    summarize(options.ltp_out_file, options.test_timings)


if __name__ == '__main__':
    main(sys.argv)
