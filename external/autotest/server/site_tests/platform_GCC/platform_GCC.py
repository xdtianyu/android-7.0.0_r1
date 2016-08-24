# Copyright (c) 2009 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import glob, logging, os, shutil
from autotest_lib.client.common_lib import error
from autotest_lib.server import test, utils
from optparse import OptionParser

class platform_GCC(test.test):
    """Class for running the GCC dejagnu tests."""
    version = 1
    results = {}

    TEST_STATUSES = ('PASS', 'FAIL', 'UNRESOLVED', 'UNTESTED', 'UNSUPPORTED',
                     'XFAIL', 'KFAIL', 'XPASS', 'KPASS')
    TARBALL = '/usr/local/dejagnu/gcc/tests.tar.gz'

    def parse_log(self, log):
        results = {}
        counts = {}
        log_file = open(log, 'rb')
        for line in log_file:
            if line.startswith(self.TEST_STATUSES):
                result, testname = line.split(': ', 1)
                testname = testname.strip()
                if testname in results:
                    counts[testname] += 1
                    unique_testname = '%s (%d)' % (testname, counts[testname])
                else:
                    counts[testname] = 1
                    unique_testname = testname
                results[unique_testname] = result
        log_file.close()
        return results


    def compare_logs(self, baseline, new):
        baseline_results = self.parse_log(baseline)
        logging.info('%d results parsed in baseline (%s).' %
                     (len(baseline_results), baseline))
        new_results = self.parse_log(new)
        logging.info('%d results parsed in new log (%s).' %
                     (len(new_results), new))

        differences = []
        for testname in new_results.keys():
            if testname not in baseline_results:
                differences.append((testname, 'NOTEXECUTED',
                                    new_results[testname]))
            elif new_results[testname] != baseline_results[testname]:
                differences.append((testname, baseline_results[testname],
                                    new_results[testname]))
        for testname in baseline_results.keys():
            if testname not in new_results:
                differences.append((testname, baseline_results[testname],
                                    'NOTEXECUTED'))
        return differences


    def run_once(self, host=None, args=[]):
        self.client = host

        parser = OptionParser()
        parser.add_option('--gcc_dir',
                          dest='gcc_dir',
                          default='/var/tmp/portage/cross-*/gcc-*/work/gcc-*build*',
                          help='Path to the gcc build directory.')
        parser.add_option('--test_flags',
                          dest='test_flags',
                          default='',
                          help='Options to pass to dejagnu.')

        options, args = parser.parse_args(args)

        utils.system('%s %s' %
                     (os.path.join(self.bindir, 'dejagnu_init_remote'),
                      self.client.ip))

        gcc_dirs = glob.glob(options.gcc_dir)
        if len(gcc_dirs) == 0:
            # If there is no directory present, try untarring the tarball
            # installed by the gcc package.
            logging.info('No gcc directory found, attempting to untar from %s'
                         % self.TARBALL)
            os.chdir('/')
            os.system('tar -xzf %s' % self.TARBALL)
            gcc_dirs = glob.glob(options.gcc_dir)
            if len(gcc_dirs) == 0:
                raise error.TestFail('No gcc directory to test was found')

        gcc_dir = gcc_dirs[0]

        logging.info('Testing gcc in the following directory: %s' % gcc_dir)
        exp_file = os.path.join(self.bindir, 'site.exp')
        client_hostname = str(self.client.ip)
        test_flags = options.test_flags
        test_command = ('cd %s; DEJAGNU="%s" DEJAGNU_SCRIPTS=%s '
                        'DEJAGNU_HOSTNAME=%s make '
                        'RUNTESTFLAGS="%s" check-gcc' % (gcc_dir, exp_file,
                        self.bindir, client_hostname, test_flags))
        utils.system(test_command)

        error_messages = []
        for log in ('gcc', 'g++'):
            log_from = os.path.join(gcc_dir, 'gcc/testsuite/%s/%s.log' %
                                    (log, log))
            log_to = os.path.join(self.resultsdir, '%s.log' % (log))
            shutil.copy(log_from, log_to)

            baseline = os.path.join(self.bindir, '%s.log' % (log))

            differences = self.compare_logs(baseline, log_to)
            for difference in differences:
                error_string = ('(%s) "%s" Expected: "%s" Actual: "%s"' %
                                (log_to, difference[0],
                                 difference[1], difference[2]))
                error_messages.append(error_string)
            keyname = log.replace('+', 'p')
            self.results['%s_differences' % keyname] = len(differences)

        self.write_perf_keyval(self.results)

        if len(error_messages) != 0:
            raise error.TestFail('\n'.join(error_messages))

    def cleanup(self):
        utils.system(os.path.join(self.bindir, 'dejagnu_cleanup_remote'))
