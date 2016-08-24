#!/usr/bin/python -u
#
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# Site extension of the default parser. Generate JSON reports and stack traces.
#
# This site parser is used to generate a JSON report of test failures, crashes,
# and the associated logs for later consumption by an Email generator. If any
# crashes are found, the debug symbols for the build are retrieved (either from
# Google Storage or local cache) and core dumps are symbolized.
#
# The parser uses the test report generator which comes bundled with the Chrome
# OS source tree in order to maintain consistency. As well as not having to keep
# track of any secondary failure white lists.
#
# Stack trace generation is done by the minidump_stackwalk utility which is also
# bundled with the Chrome OS source tree. Requires gsutil and cros_sdk utilties
# be present in the path.
#
# The path to the Chrome OS source tree is defined in global_config under the
# CROS section as 'source_tree'.
#
# Existing parse behavior is kept completely intact. If the site parser is not
# configured it will print a debug message and exit after default parser is
# called.
#

import errno, os, json, shutil, sys, tempfile, time

import common
from autotest_lib.client.bin import os_dep, utils
from autotest_lib.client.common_lib import global_config
from autotest_lib.tko import models, parse, utils as tko_utils
from autotest_lib.tko.parsers import version_0


# Name of the report file to produce upon completion.
_JSON_REPORT_FILE = 'results.json'

# Number of log lines to include from error log with each test results.
_ERROR_LOG_LIMIT = 10

# Status information is generally more useful than error log, so provide a lot.
_STATUS_LOG_LIMIT = 50


class StackTrace(object):
    """Handles all stack trace generation related duties. See generate()."""

    # Cache dir relative to chroot.
    _CACHE_DIR = 'tmp/symbol-cache'

    # Flag file indicating symbols have completed processing. One is created in
    # each new symbols directory.
    _COMPLETE_FILE = '.completed'

    # Maximum cache age in days; all older cache entries will be deleted.
    _MAX_CACHE_AGE_DAYS = 1

    # Directory inside of tarball under which the actual symbols are stored.
    _SYMBOL_DIR = 'debug/breakpad'

    # Maximum time to wait for another instance to finish processing symbols.
    _SYMBOL_WAIT_TIMEOUT = 10 * 60


    def __init__(self, results_dir, cros_src_dir):
        """Initializes class variables.

        Args:
            results_dir: Full path to the results directory to process.
            cros_src_dir: Full path to Chrome OS source tree. Must have a
                working chroot.
        """
        self._results_dir = results_dir
        self._cros_src_dir = cros_src_dir
        self._chroot_dir = os.path.join(self._cros_src_dir, 'chroot')


    def _get_cache_dir(self):
        """Returns a path to the local cache dir, creating if nonexistent.

        Symbol cache is kept inside the chroot so we don't have to mount it into
        chroot for symbol generation each time.

        Returns:
            A path to the local cache dir.
        """
        cache_dir = os.path.join(self._chroot_dir, self._CACHE_DIR)
        if not os.path.exists(cache_dir):
            try:
                os.makedirs(cache_dir)
            except OSError, e:
                if e.errno != errno.EEXIST:
                    raise
        return cache_dir


    def _get_job_name(self):
        """Returns job name read from 'label' keyval in the results dir.

        Returns:
            Job name string.
        """
        return models.job.read_keyval(self._results_dir).get('label')


    def _parse_job_name(self, job_name):
        """Returns a tuple of (board, rev, version) parsed from the job name.

        Handles job names of the form "<board-rev>-<version>...",
        "<board-rev>-<rev>-<version>...", and
        "<board-rev>-<rev>-<version_0>_to_<version>..."

        Args:
            job_name: A job name of the format detailed above.

        Returns:
            A tuple of (board, rev, version) parsed from the job name.
        """
        version = job_name.rsplit('-', 3)[1].split('_')[-1]
        arch, board, rev = job_name.split('-', 3)[:3]
        return '-'.join([arch, board]), rev, version


def parse_reason(path):
    """Process status.log or status and return a test-name: reason dict."""
    status_log = os.path.join(path, 'status.log')
    if not os.path.exists(status_log):
        status_log = os.path.join(path, 'status')
    if not os.path.exists(status_log):
        return

    reasons = {}
    last_test = None
    for line in open(status_log).readlines():
        try:
            # Since we just want the status line parser, it's okay to use the
            # version_0 parser directly; all other parsers extend it.
            status = version_0.status_line.parse_line(line)
        except:
            status = None

        # Assemble multi-line reasons into a single reason.
        if not status and last_test:
            reasons[last_test] += line

        # Skip non-lines, empty lines, and successful tests.
        if not status or not status.reason.strip() or status.status == 'GOOD':
            continue

        # Update last_test name, so we know which reason to append multi-line
        # reasons to.
        last_test = status.testname
        reasons[last_test] = status.reason

    return reasons


def main():
    # Call the original parser.
    parse.main()

    # Results directory should be the last argument passed in.
    results_dir = sys.argv[-1]

    # Load the Chrome OS source tree location.
    cros_src_dir = global_config.global_config.get_config_value(
        'CROS', 'source_tree', default='')

    # We want the standard Autotest parser to keep working even if we haven't
    # been setup properly.
    if not cros_src_dir:
        tko_utils.dprint(
            'Unable to load required components for site parser. Falling back'
            ' to default parser.')
        return

    # Load ResultCollector from the Chrome OS source tree.
    sys.path.append(os.path.join(
        cros_src_dir, 'src/platform/crostestutils/utils_py'))
    from generate_test_report import ResultCollector

    # Collect results using the standard Chrome OS test report generator. Doing
    # so allows us to use the same crash white list and reporting standards the
    # VM based test instances use.
    # TODO(scottz): Reevaluate this code usage. crosbug.com/35282
    results = ResultCollector().RecursivelyCollectResults(results_dir)
    # We don't care about successful tests. We only want failed or crashing.
    # Note: list([]) generates a copy of the dictionary, so it's safe to delete.
    for test_status in list(results):
        if test_status['crashes']:
            continue
        elif test_status['status'] == 'PASS':
            results.remove(test_status)

    # Filter results and collect logs. If we can't find a log for the test, skip
    # it. The Emailer will fill in the blanks using Database data later.
    filtered_results = {}
    for test_dict in results:
        result_log = ''
        test_name = os.path.basename(test_dict['testdir'])
        error = os.path.join(
                test_dict['testdir'], 'debug', '%s.ERROR' % test_name)

        # If the error log doesn't exist, we don't care about this test.
        if not os.path.isfile(error):
            continue

        # Parse failure reason for this test.
        for t, r in parse_reason(test_dict['testdir']).iteritems():
            # Server tests may have subtests which will each have their own
            # reason, so display the test name for the subtest in that case.
            if t != test_name:
                result_log += '%s: ' % t
            result_log += '%s\n\n' % r.strip()

        # Trim results_log to last _STATUS_LOG_LIMIT lines.
        short_result_log = '\n'.join(
            result_log.splitlines()[-1 * _STATUS_LOG_LIMIT:]).strip()

        # Let the reader know we've trimmed the log.
        if short_result_log != result_log.strip():
            short_result_log = (
                '[...displaying only the last %d status log lines...]\n%s' % (
                    _STATUS_LOG_LIMIT, short_result_log))

        # Pull out only the last _LOG_LIMIT lines of the file.
        short_log = utils.system_output('tail -n %d %s' % (
            _ERROR_LOG_LIMIT, error))

        # Let the reader know we've trimmed the log.
        if len(short_log.splitlines()) == _ERROR_LOG_LIMIT:
            short_log = (
                '[...displaying only the last %d error log lines...]\n%s' % (
                    _ERROR_LOG_LIMIT, short_log))

        filtered_results[test_name] = test_dict
        filtered_results[test_name]['log'] = '%s\n\n%s' % (
            short_result_log, short_log)

    # Generate JSON dump of results. Store in results dir.
    json_file = open(os.path.join(results_dir, _JSON_REPORT_FILE), 'w')
    json.dump(filtered_results, json_file)
    json_file.close()


if __name__ == '__main__':
    main()
