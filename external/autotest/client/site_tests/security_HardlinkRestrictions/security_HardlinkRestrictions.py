# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
#    Based on tests from http://bazaar.launchpad.net/~ubuntu-bugcontrol/qa-regression-testing/master/view/head:/scripts/test-kernel-security.py
#    Copyright (C) 2008-2011 Canonical Ltd.
#
#    This program is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License version 3,
#    as published by the Free Software Foundation.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program. If not, see <http://www.gnu.org/licenses/>.

import pwd
import tempfile
import shutil
import logging, os
from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error

class security_HardlinkRestrictions(test.test):
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

    def _is_readable(self, path, user, expected=True):
        rc = utils.system("su -c 'cat %s' %s" % (path, user),
                          ignore_status=True)
        status = (rc == 0)

        if status != expected:
            if expected:
                self._failed("'%s' was unable to read file '%s'" %
                             (user, path))
            else:
                self._failed("'%s' was able to read file '%s'" %
                             (user, path))
        return status

    def _is_writable(self, path, user, expected=True):
        rc = utils.system("su -c 'echo > %s' %s" % (path, user),
                          ignore_status=True)
        status = (rc == 0)

        if status != expected:
            if expected:
                self._failed("'%s' was unable to write file '%s'" %
                             (user, path))
            else:
                self._failed("'%s' was able to write file '%s'" %
                             (user, path))
        return status

    def _can_hardlink(self, source, target, user, expected=True):
        rc = utils.system("su -c 'ln %s %s' %s" % (source, target, user),
                          ignore_status=True)
        status = (rc == 0)

        if status != expected:
            if expected:
                self._failed("'%s' was unable to hardlink file '%s' as '%s'" %
                             (user, source, target))
            else:
                self._failed("'%s' was able to hardlink file '%s' as '%s'" %
                             (user, source, target))

        # Check and clean up hardlink if it was created.
        if os.path.exists(target):
            if not expected:
                self._failed("'%s' was able to create hardlink '%s' to '%s'" %
                             (user, target, source))
            os.unlink(target)

        return status

    def _check_hardlinks(self, user):
        uid = pwd.getpwnam(user)[2]

        # Verify we have a distinct user.
        if uid == 0:
            self._failed("The '%s' user is root(%d)!" % (user, uid))
            return

        # Build a world-writable directory, owned by user.
        tmpdir = tempfile.mkdtemp(prefix='hardlinks-')
        self._rmdir.append(tmpdir)
        os.chown(tmpdir, uid, 0)

        # Create test target files.
        secret = tempfile.NamedTemporaryFile(prefix="secret-")
        readable = tempfile.NamedTemporaryFile(prefix="readable-")
        os.chmod(readable.name, 0444)
        available = tempfile.NamedTemporaryFile(prefix="available-")
        os.chmod(available.name, 0666)

        # Verify secret target is unreadable/unwritable.
        self._is_readable(secret.name, user, expected=False)
        self._is_writable(secret.name, user, expected=False)
        # Verify readable target is only readable.
        self._is_readable(readable.name, user)
        self._is_writable(readable.name, user, expected=False)
        # Verify available target is both readable/writable.
        self._is_readable(available.name, user)
        self._is_writable(available.name, user)

        # Create pathnames for hardlinks.
        mine = os.path.join(tmpdir, 'mine')
        evil = os.path.join(tmpdir, 'evil')
        not_evil = os.path.join(tmpdir, 'not-evil')

        # Allow hardlink to files owned by the user, or writable.
        self._is_writable(mine, user)
        self._can_hardlink(mine, not_evil, user)
        self._can_hardlink(available.name, not_evil, user)

        # Disallow hardlinking to unwritable or unreadlabe files.
        self._can_hardlink(readable.name, evil, user, expected=False)
        self._can_hardlink(secret.name, evil, user, expected=False)

        # Disallow hardlinks to unowned non-regular files. This uses
        # /dev because the other locations are mounted nodev, which
        # will cause the read/write tests to fail.
        devdir = tempfile.mkdtemp(prefix="hardlinks-", dir="/dev")
        self._rmdir.append(devdir)
        os.chown(devdir, uid, 0)
        null = os.path.join(devdir, "null")
        dev_evil = os.path.join(devdir, "evil")
        dev_not_evil = os.path.join(devdir, "not-evil")
        utils.system("mknod -m 0666 %s c 1 3" % (null))
        self._is_readable(null, user)
        self._is_writable(null, user)
        self._can_hardlink(null, dev_evil, user, expected=False)

        # Allow hardlinks to owned non-regular files.
        os.chown(null, uid, 0)
        self._can_hardlink(null, dev_not_evil, user)

        # Allow CAP_FOWNER to hardlink non-regular files.
        self._can_hardlink(null, dev_not_evil, "root")

    def run_once(self):
        # Empty failure list means test passes.
        self._failures = []

        # Prepare list of directories to clean up.
        self._rmdir = []

        # Verify hardlink restrictions sysctl exists and is enabled.
        sysctl = "/proc/sys/fs/protected_hardlinks"
        if (not os.path.exists(sysctl)):
            # Fall back to looking for Yama link restriction sysctl.
            sysctl = "/proc/sys/kernel/yama/protected_nonaccess_hardlinks"
        self.check(os.path.exists(sysctl), "%s exists" % (sysctl), fatal=True)
        self.check(open(sysctl).read() == '1\n', "%s enabled" % (sysctl),
                   fatal=True)

        # Test the basic "user hardlinks to unwritable source" situation
        # first, in a more auditable way than the extensive behavior tests
        # that follow.
        if os.path.exists("/tmp/evil-hardlink"):
            os.unlink("/tmp/evil-hardlink")
        rc = utils.system("su -c 'ln /etc/shadow /tmp/evil-hardlink' chronos",
                          ignore_status=True)
        if rc != 1 or os.path.exists("/tmp/evil-hardlink"):
            self._failed("chronos user was able to create malicious hardlink")

        # Test hardlink restrictions.
        self._check_hardlinks(user='chronos')

        # Clean up from the tests.
        for path in self._rmdir:
            if os.path.exists(path):
                shutil.rmtree(path, ignore_errors=True)

        # Raise a failure if anything unexpected was seen.
        if len(self._failures):
            raise error.TestFail((", ".join(self._failures)))
