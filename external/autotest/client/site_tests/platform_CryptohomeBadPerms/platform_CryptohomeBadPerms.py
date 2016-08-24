# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import cryptohome

class platform_CryptohomeBadPerms(test.test):
    version = 1
    cryptohome_proxy = None

    def require_mount_fail(self, user):
        if self.cryptohome_proxy.mount(user, 'test', create=True):
            raise error.TestFail('Mount unexpectedly succeeded for %s' % user)

    def run_once(self):
        self.cryptohome_proxy = cryptohome.CryptohomeProxy()

        # Leaf element of user path not owned by user.
        user = utils.random_username()
        path = cryptohome.user_path(user)
        os.mkdir(path)
        os.chown(path, 0, 0)
        try:
            self.require_mount_fail(user)
        finally:
            os.rmdir(path)

        # Leaf element of system path not owned by root.
        user = utils.random_username()
        path = cryptohome.system_path(user)
        os.mkdir(path)
        os.chown(path, 1, 1)
        self.require_mount_fail(user)
        try:
            self.require_mount_fail(user)
        finally:
            os.rmdir(path)

        # Leaf element of path too permissive.
        user = utils.random_username()
        path = cryptohome.user_path(user)
        os.mkdir(path)
        os.chmod(path, 0777)
        self.require_mount_fail(user)
        try:
            self.require_mount_fail(user)
        finally:
            os.rmdir(path)

        # Non-leaf element of path not owned by root.
        user = utils.random_username()
        path = cryptohome.user_path(user)
        parent_path = os.path.dirname(path)
        os.chown(parent_path, 1, 1)
        try:
            self.require_mount_fail(user)
        finally:
            os.chown(parent_path, 0, 0)

        # Non-leaf element of path too permissive.
        user = utils.random_username()
        path = cryptohome.user_path(user)
        parent_path = os.path.dirname(path)
        os.chmod(parent_path, 0777)
        try:
            self.require_mount_fail(user)
        finally:
            os.chown(parent_path, 0, 0)
