# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from collections import namedtuple
import logging
import os

import common
from autotest_lib.client.common_lib import error
from autotest_lib.server import site_gtest_runner
from autotest_lib.server import test


NATIVE_TESTS_PATH = '/data/nativetest'
WHITELIST_FILE = '/data/nativetest/tests.txt'
LIST_TEST_BINARIES_TEMPLATE = (
        'find %(path)s -type f -mindepth 2 -maxdepth 2 '
        '\( -perm -100 -o -perm -010 -o -perm -001 \)')

GtestSuite = namedtuple('GtestSuite', ['path', 'run_as_root'])

class brillo_Gtests(test.test):
    """Run one or more native gTest Suites."""
    version = 1


    def _get_whitelisted_tests(self, whitelist_path):
        """Return the list of whitelisted tests.

        The whitelist is expected to be a two column CSV file containing the
        test name and "yes" or "no" whether the test should be run as root or
        not.
        Anything after a # on a line is considered to be a comment and  ignored.

        @param whitelist_path: Path to the whitelist.

        @return a list of GtestSuite tuples.
        """
        suites = []
        for line in self.host.run_output(
                'cat %s' % whitelist_path).splitlines():
            # Remove anything after the first # (comments).
            line = line.split('#')[0]
            if line.strip() == '':
                continue

            parts = line.split(',')
            if len(parts) != 2:
                logging.error('badly formatted line in %s: %s', whitelist_path,
                              line)
                continue

            name = parts[0].strip()
            path = os.path.join(NATIVE_TESTS_PATH, name, name)
            suites.append(GtestSuite(path, parts[1].strip() == 'yes'))
        return suites


    def _find_all_gtestsuites(self, use_whitelist=False):
        """Find all the gTest Suites installed on the DUT.

        @param use_whitelist: Only whitelisted tests found on the system will
                              be used.
        """
        list_cmd = LIST_TEST_BINARIES_TEMPLATE % {'path': NATIVE_TESTS_PATH}
        gtest_suites_path = self.host.run_output(list_cmd).splitlines()
        gtest_suites = [GtestSuite(path, True) for path in gtest_suites_path]

        if use_whitelist:
            try:
                whitelisted = self._get_whitelisted_tests(WHITELIST_FILE)
                gtest_suites = [t for t in whitelisted
                                if t.path in gtest_suites_path]
            except error.AutoservRunError:
                logging.error('Failed to read whitelist %s', WHITELIST_FILE)

        if not gtest_suites:
            raise error.TestWarn('No test executables found on the DUT')
        logging.debug('Test executables found:\n%s',
                      '\n'.join([str(t) for t in gtest_suites]))
        return gtest_suites


    def run_gtestsuite(self, gtestSuite):
        """Run a gTest Suite.

        @param gtestSuite: GtestSuite tuple.

        @return True if the all the tests in the gTest Suite pass. False
                otherwise.
        """
        # Make sure the gTest Suite exists.
        result = self.host.run('test -e %s' % gtestSuite.path,
                               ignore_status=True)
        if not result.exit_status == 0:
            logging.error('Unable to find %s', gtestSuite.path)
            return False

        result = self.host.run('test -x %s' % gtestSuite.path,
                               ignore_status=True)
        if not result.exit_status == 0:
            self.host.run('chmod +x %s' % gtestSuite.path)

        logging.debug('Running: %s', gtestSuite)
        command = gtestSuite.path
        if not gtestSuite.run_as_root:
          command = 'su shell %s' % command

        result = self.host.run(command, ignore_status=True)
        logging.debug(result.stdout)

        parser = site_gtest_runner.gtest_parser()
        for line in result.stdout.splitlines():
            parser.ProcessLogLine(line)
        passed_tests = parser.PassedTests()
        if passed_tests:
            logging.debug('Passed Tests: %s', passed_tests)
        failed_tests = parser.FailedTests(include_fails=True,
                                          include_flaky=True)
        if failed_tests:
            logging.error('Failed Tests: %s', failed_tests)
            for test in failed_tests:
                logging.error('Test %s failed:\n%s', test,
                              parser.FailureDescription(test))
            return False
        if result.exit_status != 0:
            logging.error('%s exited with exit code: %s',
                          gtestSuite, result.exit_status)
            return False
        return True


    def run_once(self, host=None, gtest_suites=None, use_whitelist=False):
        """Run gTest Suites on the DUT.

        @param host: host object representing the device under test.
        @param gtest_suites: List of gTest suites to run. Default is to run
                             every gTest suite on the host.
        @param use_whitelist: If gTestSuites is not passed in and use_whitelist
                              is true, only whitelisted tests found on the
                              system will be used.

        @raise TestFail: The test failed.
        """
        self.host = host
        if not gtest_suites:
            gtest_suites = self._find_all_gtestsuites(
                    use_whitelist=use_whitelist)

        failed_gtest_suites = []
        for gtestSuite in gtest_suites:
            if not self.run_gtestsuite(gtestSuite):
                failed_gtest_suites.append(gtestSuite)

        if failed_gtest_suites:
            logging.error(
                    'The following gTest Suites failed: \n %s',
                    '\n'.join([str(t) for t in failed_gtest_suites]))
            raise error.TestFail(
                    'Not all gTest Suites completed successfully. '
                    '%s out of %s suites failed. '
                    'Failed Suites: %s'
                    % (len(failed_gtest_suites),
                       len(gtest_suites),
                       failed_gtest_suites))
