# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os, re

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error, utils
from autotest_lib.client.cros.graphics import graphics_utils

class graphics_Gbm(test.test):
    """
    Test the gbm implementation.
    """
    version = 1
    preserve_srcdir = True
    GSC = None

    def setup(self):
        os.chdir(self.srcdir)
        utils.make('clean')
        utils.make('all')

    def initialize(self):
        self.GSC = graphics_utils.GraphicsStateChecker()

    def cleanup(self):
        if self.GSC:
            self.GSC.finalize()

    def run_once(self):
        cmd = os.path.join(self.srcdir, 'gbmtest')
        cmd = graphics_utils.xcommand(cmd)
        result = utils.run(cmd,
                           stderr_is_expected = False,
                           stdout_tee = utils.TEE_TO_LOGS,
                           stderr_tee = utils.TEE_TO_LOGS,
                           ignore_status = True)
        report = re.findall(r'\[  PASSED  \]', result.stdout)
        if not report:
            raise error.TestFail('Gbm test failed (' + result.stdout + ')')

