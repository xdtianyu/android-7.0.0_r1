#!/usr/bin/python
#
# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, os, tempfile
from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error

class security_mprotect(test.test):
    """
    Verify mprotect of PROT_EXEC works on noexec mounts.
    """
    version = 1
    executable = 'prot_exec'


    def setup(self):
        os.chdir(self.srcdir)
        utils.make(self.executable)


    def run_once(self):
        with tempfile.NamedTemporaryFile(prefix='%s-' % (self.executable),
                                         dir='/run', delete=True) as temp:
            temp_file_name = temp.name

        r = utils.run("%s/%s %s" % (self.srcdir, self.executable,
                                    temp_file_name),
                      stdout_tee=utils.TEE_TO_LOGS,
                      stderr_tee=utils.TEE_TO_LOGS,
                      ignore_status=True)
        if r.exit_status != 0 or len(r.stderr) > 0:
            raise error.TestFail(r.stderr)
        if 'skipping' in r.stdout:
            logging.debug(r.stdout)
            return
        if 'pass' not in r.stdout:
            raise error.TestFail(r.stdout)
