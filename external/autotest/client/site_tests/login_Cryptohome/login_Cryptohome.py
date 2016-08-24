# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import chrome
from autotest_lib.client.cros import cryptohome

TEST_USER = 'cryptohome_test@chromium.org'
TEST_PASS = 'testme'

class login_Cryptohome(test.test):
    """Verify the cryptohome is mounted only after login."""
    version = 1


    def run_once(self):
        username = ''
        with chrome.Chrome() as cr:
            username = cr.username
            if not cryptohome.is_vault_mounted(user=username,
                                               allow_fail=False):
                raise error.TestFail('Expected to find a mounted vault.')

        if cryptohome.is_vault_mounted(user=username,
                                       allow_fail=True):
            raise error.TestFail('Expected to not find a mounted vault.')

        # Remove our vault, mount another vault, create a test file
        # in the other vault, and ensure that the file no longer exists
        # after we log back in.
        cryptohome.remove_vault(username)

        cryptohome.mount_vault(TEST_USER, TEST_PASS, create=True)
        test_file = os.path.join(cryptohome.user_path(TEST_USER), 'hello')
        open(test_file, 'w').close()
        cryptohome.unmount_vault(TEST_USER)

        with chrome.Chrome():
            if not cryptohome.is_vault_mounted(user=username,
                                               allow_fail=False):
                raise error.TestFail('Expected to find user\'s mounted vault.')
            if os.path.exists(test_file):
                raise error.TestFail('Expected to not find the test file.')
