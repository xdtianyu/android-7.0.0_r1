# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
from autotest_lib.client.bin import utils, test
from autotest_lib.client.common_lib import error

import parse_ltp_out


class kernel_LTP(test.test):
    """Base class ltp test runner."""
    _DEP = 'kernel_ltp_dep'
    version = 1

    # Note: to run specific test(s), runltp supports the following options:
    #       -f CMDFILES (separate with ',')
    #       -s PATTERN
    #       -S SKIPFILE (ChromeOS uses ./site_excluded)
    #
    #       CMDFILES are lists of tests grouped by area. If no CMDFILES
    #       are supplied, runltp has a default set of CMDFILES listed
    #       in '$LTPROOT/scenario_groups/default' it uses. The CMDFILES are
    #       individual files under '$LTPROOT/runtest' such as commands, dio
    #       and fsx. Finally, the test cases listed in a CMDFILE are individual
    #       tests which reside under '$LTPROOT/testcases'.
    #
    #       Then, an abridged look at the parts of the LTP dir structure
    #       used here would be:
    #       $LTPROOT
    #               /scenario_groups/default - default list of CMDFILES
    #               /runtest/...             - CMDFILES group test cases
    #               /testcases/...           - test cases
    #
    #       The PATTERN argument is used to refine the tests that will run
    #       within an individual CMDFILE by supplying a regex match for
    #       individual test case names.
    #
    #       The SKIPFILE lists individual test cases to be excluded. These
    #       tests are not appropriate for a project.  ChromeOS uses the
    #       default SKIPFILE=site_excluded.
    #
    # e.g. -for all tests in math cmdfile:
    #           job.run_test('ltp', '-f math')
    #      -for just the float_bessel test in the math cmdfile:
    #           job.run_test('ltp', '-f math -s float_bessel')
    #      -for the math and memory management cmdfiles:
    #           job.run_test('ltp', '-f math,mm')
    def run_once(self, args='', script='runltp', select_tests=None):
        """A test wrapper for running tests/scripts under $LTPROOT.

        For ChromeOS $LTPROOT is the repo under src/third_party/ltp.

        @param args: arguments to be passed to 'script' (usually runltp).
        @param script: LTP script to run.
        @param select_tests: comma-separated list of names of tests
                             (executable files under ltp/testcases/bin) to run.
                             Used for running and debugging during development.
        """
        # In case the user wants to run a test script other than runltp
        # though runltp is the common case.
        if script == 'runltp':
            failcmdfile = os.path.join(self.debugdir, 'failcmdfile')
            outfile = os.path.join(self.resultsdir, 'ltp.out')
            args2 = ['-l %s' % os.path.join(self.resultsdir, 'ltp.log'),
                     '-C %s' % failcmdfile,
                     '-d %s' % self.tmpdir,
                     '-o %s' % outfile,
                     '-S %s' % os.path.join(self.bindir, 'site_excluded')]
            args = '%s -p %s' % (args, ' '.join(args2))

        # Uses the LTP binaries build into client/deps/kernel_ltp_dep.
        dep = self._DEP
        dep_dir = os.path.join(self.autodir, 'deps', dep)
        self.job.install_pkg(dep, 'dep', dep_dir)

        # Setup a fake runtest/testcase file if only running one test.
        if select_tests:
            # Selected files must exist under testcases/bin.
            testcase_bin_dir = os.path.join(dep_dir, 'testcases', 'bin')
            select_tests = select_tests.split(',')
            for select_test in select_tests:
                test_bin_file = os.path.join(testcase_bin_dir, select_test)
                if not os.path.isfile(test_bin_file):
                    raise error.TestFail('%s not found.' % test_bin_file)
            with open(os.path.join(dep_dir, 'runtest', 'cros_suite'), 'w') as f:
                for select_test in select_tests:
                    f.write('%s %s\n' % (select_test, select_test))
            args += ' -f cros_suite'

        cmd = '%s %s' % (os.path.join(dep_dir, script), args)
        result = utils.run(cmd, ignore_status=True)

        if script == 'runltp':
            parse_ltp_out.summarize(outfile)

        # look for any failed test command.
        try:
            f = open(failcmdfile)
        except IOError:
            raise error.TestFail('Expected to find failcmdfile but did not.')
        failed_cmd = f.read().strip()
        f.close()
        if failed_cmd:
            raise error.TestFail(failed_cmd)
