# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, os

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.graphics import graphics_utils

class graphics_PiglitBVT(test.test):
    """
    Runs a slice of the passing Piglit test sets.
    """
    version = 1

    test_scripts = 'test_scripts/'
    GSC = None

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

    def run_once(self, test_slice):
        # TODO(ihf): Remove this once Piglit works on freon.
        if utils.is_freon():
            return

        gpu_family = utils.get_gpu_family()
        family = gpu_family
        logging.info('Detected gpu family %s.', gpu_family)

        # TODO(ihf): Delete this once we have a piglit that runs on ARM.
        if gpu_family in ['mali', 'tegra']:
            logging.info('Not running any tests, passing by default.')
            return

        scripts_dir = os.path.join(self.bindir, self.test_scripts)
        family_dir = os.path.join(scripts_dir, family)
        # We don't want to introduce too many combinations, so fall back.
        if not os.path.isdir(family_dir):
            family = 'other'
            family_dir = os.path.join(scripts_dir, family)
        logging.info('Using scripts for gpu family %s.', family)
        scripts_dir = os.path.join(self.bindir, self.test_scripts)
        # Mark scripts executable if they are not.
        utils.system('chmod +x ' + scripts_dir + '*/graphics_PiglitBVT_*.sh')

        # Kick off test script.
        cmd = ('source ' + os.path.join(family_dir, 'graphics_PiglitBVT_%d.sh' %
                                                    test_slice))
        logging.info('Executing cmd = %s', cmd)
        result = utils.run(cmd,
                           stdout_tee=utils.TEE_TO_LOGS,
                           stderr_tee=utils.TEE_TO_LOGS,
                           ignore_status = True)
        tests_failed = result.exit_status
        if tests_failed:
            reason = '%d tests failed on "%s" in slice %d' % (tests_failed,
                                                              gpu_family,
                                                              test_slice)
            raise error.TestError(reason)
