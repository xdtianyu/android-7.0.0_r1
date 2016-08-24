# Copyright (c) 2012 The Chromium OS Authors.
#
# Based on tests from http://bazaar.launchpad.net/~ubuntu-bugcontrol/qa-regression-testing/master/view/head:/scripts/test-kernel-security.py
# Copyright (C) 2010-2011 Canonical Ltd.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 3,
# as published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.

import logging, os
from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error

class security_ptraceRestrictions(test.test):
    version = 1

    def _passed(self, msg):
        logging.info('ok: %s' % (msg))

    def _failed(self, msg):
        logging.error('FAIL: %s' % (msg))
        self._failures.append(msg)

    def _fatal(self, msg):
        logging.error('FATAL: %s' % (msg))
        raise error.TestError(msg)

    def check(self, boolean, msg, fatal=False):
        if boolean == True:
            self._passed(msg)
        else:
            msg = "could not satisfy '%s'" % (msg)
            if fatal:
                self._fatal(msg)
            else:
                self._failed(msg)

    def setup(self):
        # Build helpers.
        os.chdir(self.srcdir)
        utils.make("clean")
        utils.make("all")

    def run_once(self):
        # Empty failure list means test passes.
        self._failures = []

        # Verify Yama exists and has ptrace restrictions enabled.
        sysctl = "/proc/sys/kernel/yama/ptrace_scope"
        self.check(os.path.exists(sysctl), "%s exists" % (sysctl), fatal=True)
        self.check(open(sysctl).read() == '1\n', "%s enabled" % (sysctl),
                   fatal=True)

        os.chdir(self.srcdir)

        # Verify ptrace is only allowed on children or declared processes.
        utils.system("su -c 'bash -x ./ptrace-restrictions.sh' chronos",
		     timeout=30)
        # Verify ptrace can be made to work across pid namespaces.
        utils.system("bash -x ./root-ptrace-restrictions.sh chronos",
		     timeout=10)
        # Verify ptrace of child ok from parent process and thread.
        utils.system("su -c './thread-prctl 0 1' chronos")
        utils.system("su -c './thread-prctl 0 0' chronos")
        # Verify prctl(PR_SET_PTRACER, ...) ok from main process and thread.
        utils.system("su -c './thread-prctl 1 1' chronos")
        utils.system("su -c './thread-prctl 2 1' chronos")
        # Verify ptrace from thread on process that used PR_SET_PTRACER.
        utils.system("su -c './thread-prctl 1 0' chronos")
        utils.system("su -c './thread-prctl 2 0' chronos")

        # Raise a failure if anything unexpected was seen.
        if len(self._failures):
            raise error.TestFail((", ".join(self._failures)))
