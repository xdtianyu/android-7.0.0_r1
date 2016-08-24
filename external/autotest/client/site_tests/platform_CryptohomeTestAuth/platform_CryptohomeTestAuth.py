# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os
import re
import shutil

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error, utils
from autotest_lib.client.cros import constants, cryptohome

class platform_CryptohomeTestAuth(test.test):
    version = 1


    def run_once(self):
        test_user = 'this_is_a_local_test_account@chromium.org'
        test_password = 'this_is_a_test_password'

        user_hash = cryptohome.get_user_hash(test_user)


        # Ensure that the user directory is unmounted and does not exist.
        cryptohome.unmount_vault(test_user)
        cryptohome.remove_vault(test_user)
        if os.path.exists(os.path.join(constants.SHADOW_ROOT, user_hash)):
            raise error.TestFail('Could not remove the test user.')

        # Mount the test user account, which ensures that the vault is
        # created, and that the mount succeeds.
        cryptohome.mount_vault(test_user, test_password, create=True)

        # Test credentials when the user's directory is mounted
        if not cryptohome.test_auth(test_user, test_password):
            raise error.TestFail('Valid credentials should authenticate '
                                 'while mounted.')

        # Make sure that an incorrect password fails
        if cryptohome.test_auth(test_user, 'badpass'):
            raise error.TestFail('Invalid credentials should not authenticate '
                                 'while mounted.')

        # Unmount the directory
        cryptohome.unmount_vault(test_user)
        # Ensure that the user directory is not mounted
        if cryptohome.is_vault_mounted(user=test_user, allow_fail=True):
            raise error.TestFail('Cryptohome did not unmount the user.')

        # Test valid credentials when the user's directory is not mounted
        if not cryptohome.test_auth(test_user, test_password):
            raise error.TestFail('Valid credentials should authenticate '
                                 ' while mounted.')

        # Test invalid credentials fails while not mounted.
        if cryptohome.test_auth(test_user, 'badpass'):
            raise error.TestFail('Invalid credentials should not authenticate '
                                 'when unmounted.')


        # Re-mount existing test user vault, verifying that the mount succeeds.
        cryptohome.mount_vault(test_user, test_password)

        # Finally, unmount and destroy the vault again.
        cryptohome.unmount_vault(test_user)
        cryptohome.remove_vault(test_user)
        if os.path.exists(os.path.join(constants.SHADOW_ROOT, user_hash)):
            raise error.TestFail('Could not destroy the vault.')
