# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# Based on tests from:
# http://bazaar.launchpad.net/~ubuntu-bugcontrol/qa-regression-testing/master/view/head:/scripts/test-kernel-security.py
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

class security_SymlinkRestrictions(test.test):
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

    def _read_contents_as(self, path, content, user, fail=False):
        cat = utils.run("su -c 'cat %s' %s" % (path, user), ignore_status=True)
        if fail:
            self.check(cat.exit_status != 0,
                       "%s not readable by %s (exit status %d)" %
                       (path, user, cat.exit_status))
            self.check(cat.stdout != content,
                       "%s not readable by %s (content '%s')" %
                       (path, user, cat.stdout))
        else:
            self.check(cat.exit_status == 0,
                       "%s readable by %s (exit status %d)" %
                       (path, user, cat.exit_status))
            self.check(cat.stdout == content,
                       "%s readable by %s (content '%s')" %
                       (path, user, cat.stdout))

    def _write_path_as(self, path, user, fail=False):
        rc = utils.system("su -c 'dd if=/etc/passwd of=%s' %s" % (path, user),
                          ignore_status=True)
        if fail:
            self.check(rc != 0, "%s unwritable by %s (exit status %d)" %
                                (path, user, rc))
        else:
            self.check(rc == 0, "%s writable by %s (exit status %d)" %
                                (path, user, rc))

    def _write_as(self, op_path, chk_path, user, create=False, fail=False):
        if create:
            if os.path.exists(chk_path):
                os.unlink(chk_path)
            self.check(os.path.exists(chk_path) == False,
                       "%s does not exist starting _write_as()" % (chk_path))
        else:
            open(chk_path, 'w').write('blah blah\n')
            self.check(os.path.exists(chk_path),
                       "%s exists" % (chk_path))
            os.chown(chk_path, pwd.getpwnam(user)[2], 0)
        self._write_path_as(op_path, user, fail=fail)
        if fail:
            if create:
                self.check(not os.path.exists(chk_path),
                           "%s does not exist at end of _write_as()" %
                           (chk_path))
        else:
            self.check(os.path.exists(chk_path),
                       "%s exists at end of _write_as()" % (chk_path))
            self.check(os.stat(chk_path).st_uid == pwd.getpwnam(user)[2],
                       "%s owned by %s at end of _write_as()" %
                       (chk_path, user))

    def _check_symlinks(self, sticky, userone, usertwo):
        uidone = pwd.getpwnam(userone)[2]
        uidtwo = pwd.getpwnam(usertwo)[2]

        # Verify we have distinct users.
        if userone == usertwo:
            self._failed("The '%s' and '%s' user have the same name!" %
                         userone, usertwo)
            return
        if uidone == uidtwo:
            self._failed("The '%s' and '%s' user have the same uid!" %
                         userone, usertwo)
            return

        # Build a world-writable directory, owned by userone.
        prefix = 'symlinks-'
        if not sticky:
            prefix += 'not'
        prefix += 'sticky-'
        tmpdir = tempfile.mkdtemp(prefix=prefix)
        self._rmdir.append(tmpdir)
        mode = 0777
        if sticky:
            mode |= 01000
        os.chmod(tmpdir, mode)
        os.chown(tmpdir, uidone, 0)

        # Verify stickiness behavior, taking uid0's DAC_OVERRIDE into account.
        drop = os.path.join(tmpdir, "remove.me")
        open(drop, 'w').write("I can be deleted in a non-sticky directory")
        os.chown(drop, uidone, 0)

        expected = 0
        if sticky and (uidtwo != 0):
            expected = 1
        rc = utils.system("su -c 'rm -f %s' %s" % (drop, usertwo),
                          ignore_status=True)
        if rc != expected:
            if sticky:
                self._failed("'%s' was able to delete files owned by '%s' "
                             "in a sticky world-writable directory" %
                             (usertwo, userone))
            else:
                self._failed("'%s' wasn't able to delete files owned by '%s' "
                             "in a regular world-writable directory" %
                             (usertwo, userone))
            return
        # File should still exist in a sticky directory.
        self.check(os.path.exists(drop) == (sticky and uidtwo != 0),
                   "'%s' should only exist in a sticky directory" % (drop))

        # Create target files.
        message = 'not very sekrit'
        target = os.path.join(tmpdir, 'target')
        open(target, 'w').write(message)
        os.chmod(target, 0644)

        sekrit_userone = 'sekrit %s' % (userone)
        target_userone = os.path.join(tmpdir, 'target-%s' % (userone))
        open(target_userone, 'w').write(sekrit_userone)
        os.chmod(target_userone, 0400)
        os.chown(target_userone, uidone, 0)

        sekrit_usertwo = 'sekrit %s' % (usertwo)
        target_usertwo = os.path.join(tmpdir, 'target-%s' % (usertwo))
        open(target_usertwo, 'w').write(sekrit_usertwo)
        os.chmod(target_usertwo, 0400)
        os.chown(target_usertwo, uidtwo, 0)

        # Create symlinks to target as different users.
        userone_symlink = os.path.join(tmpdir, '%s.symlink' % (userone))
        usertwo_symlink = os.path.join(tmpdir, '%s.symlink' % (usertwo))

        utils.system("su -c 'ln -s %s %s' %s" % (target, userone_symlink,
                                                 userone))
        utils.system("su -c 'ln -s %s %s' %s" % (target, usertwo_symlink,
                                                 usertwo))
        self.check(os.lstat(userone_symlink).st_uid == uidone,
                   "%s owned by %s" % (userone_symlink, userone))
        self.check(os.lstat(usertwo_symlink).st_uid == uidtwo,
                   "%s owned by %s" % (usertwo_symlink, usertwo))
        # Verify userone symlink and directory are owned by the same uid.
        self.check(os.lstat(userone_symlink).st_uid == os.lstat(tmpdir).st_uid,
                   "%s and %s have same owner" %
                   (tmpdir, userone_symlink))

        ## Perform read verifications.
        # Global target should be directly readable by both users.
        self._read_contents_as(target, message, userone)
        self._read_contents_as(target, message, usertwo)
        # Individual targets should only be readable by owner, verifying
        # DAC sanity, before we check symlink restriction tweaks, though
        # we have to account for uid0's DAC_OVERRIDE.
        self._read_contents_as(target_userone, sekrit_userone, userone)
        self._read_contents_as(target_usertwo, sekrit_usertwo, usertwo)
        self._read_contents_as(target_userone, sekrit_userone,
                                usertwo, fail=(uidtwo != 0))
        self._read_contents_as(target_usertwo, sekrit_usertwo,
                                userone, fail=(uidone != 0))
        # Global target should be readable through symlink by symlink owner,
        self._read_contents_as(userone_symlink, message, userone)
        self._read_contents_as(usertwo_symlink, message, usertwo)
        # Global target should be readable through symlink of directory owner.
        self._read_contents_as(userone_symlink, message, usertwo)
        # Global target should not be readable through symlink when directory
        # is sticky and the symlink and directory owner are different.
        self._read_contents_as(usertwo_symlink, message, userone,
                               fail=sticky)

        ## Perform write verifications.
        # Global target should be directly writable by both users.
        self._write_as(target, target, userone)
        self._write_as(target, target, usertwo)
        # Global target should be writable through owner's symlink.
        self._write_as(userone_symlink, target, userone)
        self._write_as(usertwo_symlink, target, usertwo)
        # Global target should be writable through symlink of directory owner.
        self._write_as(userone_symlink, target, usertwo)
        # Global target should be unwritable through symlink when directory
        # is sticky and the symlink and directory owner are different.
        self._write_as(usertwo_symlink, target, userone, fail=sticky)

        ## Perform write-with-create verifications.
        # Global target should be directly creatable by both users.
        self._write_as(target, target, userone, create=True)
        self._write_as(target, target, usertwo, create=True)
        # Global target should be creatable through owner's symlink.
        self._write_as(userone_symlink, target, userone, create=True)
        self._write_as(usertwo_symlink, target, usertwo, create=True)
        # Global target should be creatable through symlink of directory owner.
        self._write_as(userone_symlink, target, usertwo, create=True)
        # Global target should be uncreatable through symlink when directory
        # is sticky and the symlink and directory owner are different.
        self._write_as(usertwo_symlink, target, userone, create=True,
                       fail=sticky)

    def run_once(self):
        # Empty failure list means test passes.
        self._failures = []

        # Prepare list of directories to clean up.
        self._rmdir = []

        # Verify symlink restrictions sysctl exists and is enabled.
        sysctl = "/proc/sys/fs/protected_symlinks"
        if (not os.path.exists(sysctl)):
            # Fall back to looking for Yama link restriction sysctl.
            sysctl = "/proc/sys/kernel/yama/protected_sticky_symlinks"
        self.check(os.path.exists(sysctl), "%s exists" % (sysctl), fatal=True)
        self.check(open(sysctl).read() == '1\n', "%s enabled" % (sysctl),
                   fatal=True)

        # Test the basic "root follows evil symlink" situation first, in
        # a more auditable way than the extensive behavior tests that follow.
        if os.path.exists("/tmp/evil-symlink"):
            os.unlink("/tmp/evil-symlink")
        utils.system("su -c 'ln -s /etc/shadow /tmp/evil-symlink' chronos")
        rc = utils.system("cat /tmp/evil-symlink", ignore_status=True)
        if rc != 1:
            self._failed("root user was able to follow malicious symlink")
        os.unlink("/tmp/evil-symlink")

        # Test symlink restrictions, making sure there is no special
        # behavior for the root user (DAC_OVERRIDE is ignored).
        self._check_symlinks(sticky=False, userone='root', usertwo='chronos')
        self._check_symlinks(sticky=False, userone='chronos', usertwo='root')
        self._check_symlinks(sticky=True, userone='root', usertwo='chronos')
        self._check_symlinks(sticky=True, userone='chronos', usertwo='root')

        # Clean up from the tests.
        for path in self._rmdir:
            if os.path.exists(path):
                shutil.rmtree(path, ignore_errors=True)

        # Raise a failure if anything unexpected was seen.
        if len(self._failures):
            raise error.TestFail((", ".join(self._failures)))
