# Copyright (c) 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import kernel_config

class security_AltSyscall(test.test):
    """
    Verify that alt_syscall allows/blocks system calls as expected using
    minijail.
    """
    version = 1

    def initialize(self):
        self.job.require_gcc()

    def setup(self):
        os.chdir(self.srcdir)
        utils.make('clean')
        utils.make()

    def run_test(self, exe, table, expected_ret, pretty_msg):
        """
        Runs a single alt_syscall test case.

        Runs the executable with the specified alt_syscall table using minijail.
        Fails the test if the return value does not match what we expected.

        @param exe Test executable
        @param table Alt_syscall table name
        @param expected_ret Expected return value from the test
        @param pretty_msg Message to display on failue
        """
        cmdline = '/sbin/minijail0 -a %s %s/%s' % (table, self.srcdir, exe)

        logging.info("Command line: " + cmdline)
        ret = utils.system(cmdline, ignore_status=True)

        if ret != expected_ret:
            logging.error("ret: %d, expected: %d", ret, expected_ret)
            raise error.TestFail(pretty_msg)

    def alt_syscall_supported(self):
        """
        Check that alt_syscall is supported by the kernel.
        """
        config = kernel_config.KernelConfig()
        config.initialize()
        config.is_enabled('ALT_SYSCALL')
        config.is_enabled('ALT_SYSCALL_CHROMIUMOS')
        return len(config.failures()) == 0

    def run_once(self):
        if not self.alt_syscall_supported():
            raise error.TestFail("ALT_SYSCALL not supported")

        case_allow = ("read", "read_write_test", 0,
                      "Allowed system calls failed")
        case_deny_blocked = ("mmap", "read_write_test", 2,
                             "Blocked system calls succeeded")
        case_deny_alt_syscall = ("alt_syscall", "read_write_test", 1,
                                 "Changing alt_syscall table succeeded")

        for case in [case_allow, case_deny_blocked, case_deny_alt_syscall]:
            self.run_test(*case)
