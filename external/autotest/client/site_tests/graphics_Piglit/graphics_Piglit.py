# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""
Runs the piglit OpenGL suite of tests.
"""

import logging, os, re
from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.graphics import graphics_utils
from optparse import OptionParser

class graphics_Piglit(test.test):
    """
    Collection of automated tests for OpenGL implementations.

    The binaries are pulled into test images via media-lib/piglit.
    http://piglit.freedesktop.org
    """
    version = 2
    preserve_srcdir = True
    GSC = None
    piglit_path = '/usr/local/piglit'

    def initialize(self):
        self.GSC = graphics_utils.GraphicsStateChecker()

    def cleanup(self):
        if self.GSC:
            keyvals = self.GSC.get_memory_keyvals()
            for key, val in keyvals.iteritems():
                self.output_perf_value(description=key, value=val,
                                       units='bytes', higher_is_better=False)
            self.GSC.finalize()
            self.write_perf_keyval(keyvals)

    def run_once(self, test='cros-driver.py', args=[]):
        parser = OptionParser()
        parser.add_option('-t',
                          '--test-name',
                          dest='testName',
                          default='',
                          help='Run a specific piglit test.')
        options, args = parser.parse_args(args)
        gpu_family = utils.get_gpu_family()
        logging.info('Detected gpu family %s.', gpu_family)
        # TODO(djkurtz): Delete this once piglit runs on mali/tegra.
        if gpu_family in ['mali', 'tegra']:
            logging.info('Not running any tests, passing by default.')
            return

        # Keep a copy of stdout in piglit-run.log.
        log_path = os.path.join(self.outputdir, 'piglit-run.log')
        # Keep the html results in the cros-driver directory.
        results_path = os.path.join(self.outputdir, 'cros-driver')
        # The location of the piglit executable script.
        run_path = os.path.join(self.piglit_path, 'bin/piglit')
        summary = ''
        if not (os.path.exists(run_path)):
            raise error.TestError('piglit not found at %s' % self.piglit_path)

        os.chdir(self.piglit_path)
        logging.info('cd %s', os.getcwd())
        # Piglit by default wants to run multiple tests in separate processes
        # concurrently. Strictly serialize this using --no-concurrency.
        # Now --dmesg also implies no concurrency but we want to be explicit.
        flags = 'run -v --dmesg --no-concurrency'
        if (options.testName != ''):
            flags = flags + ' -t ' + options.testName
        cmd = 'python %s %s %s %s' % (run_path, flags, test, self.outputdir)
        # Pipe stdout and stderr into piglit-run.log for later analysis.
        cmd = cmd + ' | tee ' + log_path
        cmd = graphics_utils.xcommand(cmd)
        logging.info(cmd)
        utils.run(cmd,
                  stderr_is_expected = False,
                  stdout_tee = utils.TEE_TO_LOGS,
                  stderr_tee = utils.TEE_TO_LOGS)

        # Make sure logs get written before continuing.
        utils.run('sync')
        # Convert results.json file to human readable html.
        cmd = ('python %s summary html --overwrite -e all %s %s/results.json' %
                  (run_path, results_path, self.outputdir))
        utils.run(cmd,
                  stderr_is_expected = False,
                  stdout_tee = utils.TEE_TO_LOGS,
                  stderr_tee = utils.TEE_TO_LOGS)
        # Make sure logs get written before continuing.
        utils.run('sync')

        # Count number of pass, fail, warn and skip in piglit-run.log (could
        # also use results.json)
        f = open(log_path, 'r')
        summary = f.read()
        f.close()
        if not summary:
            raise error.TestError('Test summary was empty')

        # Output counts for future processing.
        keyvals = {}
        for k in ['pass', 'fail', 'crash', 'warn', 'skip']:
            num = len(re.findall(r'' + k + ' :: ', summary))
            keyvals['count_subtests_' + k] = num
            logging.info('Piglit: %d ' + k, num)
            self.output_perf_value(description=k, value=num,
                                   units='count', higher_is_better=(k=='pass'))

        self.write_perf_keyval(keyvals)
