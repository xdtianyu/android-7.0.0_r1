#!/usr/bin/python
#
# Copyright (c) 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, os
from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error

class security_SMMLocked(test.test):
    """
    Verify SMM has SMRAM unmapped and that the SMM registers are locked.
    """
    version = 1
    executable = 'smm'

    def setup(self):
        os.chdir(self.srcdir)
        utils.make(self.executable)

    def run_once(self):
        cpu_arch = utils.get_cpu_arch()
        if cpu_arch == "arm":
            logging.debug('ok: skipping SMM test for %s.', cpu_arch)
            return

        r = utils.run("%s/%s" % (self.srcdir, self.executable),
                      stdout_tee=utils.TEE_TO_LOGS,
                      stderr_tee=utils.TEE_TO_LOGS,
                      ignore_status=True)
        if r.exit_status != 0 or len(r.stderr) > 0:
            raise error.TestFail(r.stderr)
        if 'skipping' in r.stdout:
            logging.debug(r.stdout)
            return
        if 'ok' not in r.stdout:
            raise error.TestFail(r.stdout)
