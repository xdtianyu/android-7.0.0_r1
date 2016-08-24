# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.bin import test
from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error

import os

"""A test verifying that seccomp calls change permissions correctly.

Compiles tests written in C and then runs them.  Fails if C tests fail.
"""

class security_SeccompSyscallFilters(test.test):
    version = 1
    executable = 'seccomp_bpf_tests'

    def setup(self):
        """Cleans and makes seccomp_bpf_tests.c.

        Prepares environment for tests by removing directory we will extract
        to (if it exists), extracting tarball of tests, and making them.
        """
        os.chdir(self.srcdir)
        utils.make()

    def run_once(self):
        """Main function.

        Runs the compiled tests, logs output.  Fails if the call to run
        tests fails (meaning that a test failed). Runs both as root
        and non-root.
        """
        binpath = os.path.join(self.srcdir, self.executable)
        utils.system_output(binpath, retain_output = True)
        utils.system_output("su chronos -c %s" % binpath, retain_output = True)
