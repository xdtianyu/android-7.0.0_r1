# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import cryptohome

class platform_CryptohomeMigrateKey(test.test):
    version = 1

    def good(self):
        user = utils.random_username()
        old_pass = 'old'
        new_pass = 'new'

        if not self.proxy.mount(user, old_pass, create=True):
            raise error.TestFail('Could not create good user.')
        if not self.proxy.unmount(user):
            raise error.TestFail('Could not unmount good user.')
        if not self.proxy.migrate(user, old_pass, new_pass):
            raise error.TestFail('Could not migrate good user.')
        if self.proxy.mount(user, old_pass):
            raise error.TestFail('Old password still works.')
        if not self.proxy.mount(user, new_pass):
            raise error.TestFail('Could not mount good user.')
        if not self.proxy.unmount(user):
            raise error.TestFail('Could not unmount good user.')
        self.proxy.remove(user)

    def bad_password(self):
        user = utils.random_username()
        old_pass = 'old'
        new_pass = 'new'
        if not self.proxy.mount(user, old_pass, create=True):
            raise error.TestFail('Could not create bad user.')
        if not self.proxy.unmount(user):
            raise error.TestFail('Could not unmount bad user.')
        if self.proxy.migrate(user, 'bad', new_pass):
            raise error.TestFail('Migrated with bad password.')
        self.proxy.remove(user)

    def nonexistent_user(self):
        user = utils.random_username()
        old_pass = 'old'
        new_pass = 'new'
        if self.proxy.migrate(user, old_pass, new_pass):
            raise error.TestFail('Migration nonexistent user.')

    def run_once(self):
        self.proxy = cryptohome.CryptohomeProxy()
        self.good()
        self.bad_password()
        self.nonexistent_user()
