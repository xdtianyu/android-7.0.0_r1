# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import cryptohome

class platform_CryptohomeNonDirs(test.test):
    version = 1
    cryptohome_proxy = None

    def require_mount_fail(self, user):
        if self.cryptohome_proxy.mount(user, 'test', create=True):
            raise error.TestFail('Mount failed for %s' % user)

    def replace(self, src, dest):
        """Replaces dest with src.

        Replaces the dirent at dest with the dirent at src, deleting dest first
        if necessary. This is distinguished from os.rename() or shutil.move() by
        the fact that it works even if dest is a non-directory dirent.
        """
        if os.path.exists(dest):
            os.remove(dest)
        os.rename(src, dest)

    def run_once(self):
        self.cryptohome_proxy = cryptohome.CryptohomeProxy()

        # Leaf element of user path is non-dir.
        user = utils.random_username()
        path = cryptohome.user_path(user)
        utils.open_write_close(path, '')
        try:
            self.require_mount_fail(user)
        finally:
            os.remove(path)

        # Leaf element of system path is non-dir.
        user = utils.random_username()
        path = cryptohome.system_path(user)
        os.symlink('/etc', path)
        try:
            self.require_mount_fail(user)
        finally:
            os.remove(path)

        # Non-leaf element of user path is non-dir.
        user = utils.random_username()
        path = cryptohome.user_path(user)
        parent_path = os.path.dirname(path)
        os.rename(parent_path, parent_path + '.old')
        try:
            utils.open_write_close(parent_path, '')
            self.require_mount_fail(user)
        finally:
            # We can't just rely on the rename() to blow away the file -
            # rename() will refuse to rename directories to non-directory names.
            self.replace(parent_path + '.old', parent_path)

        # Non-leaf element of system path is non-dir.
        user = utils.random_username()
        path = cryptohome.system_path(user)
        parent_path = os.path.dirname(path)
        os.rename(parent_path, parent_path + '.old')
        try:
            utils.open_write_close(parent_path, '')
            self.require_mount_fail(user)
        finally:
            self.replace(parent_path + '.old', parent_path)
