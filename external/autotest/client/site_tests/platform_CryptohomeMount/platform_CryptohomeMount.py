# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import cryptohome

class platform_CryptohomeMount(test.test):
    """Validates basic cryptohome creation and mounting."""
    version = 1


    def run_once(self):
        test_user = 'this_is_a_local_test_account@chromium.org';
        test_password = 'this_is_a_test_password';
        # Get the hash for the test user account
        user_hash = cryptohome.get_user_hash(test_user)
        proxy = cryptohome.CryptohomeProxy()

        # Remove the test user account
        proxy.remove(test_user)

        # Mount the test user account
        if not proxy.mount(test_user, test_password, create=True):
          raise error.TestFail('Failed to create and mount the test user')

        # Unmount the directory
        if not proxy.unmount(test_user):
          raise error.TestFail('Failed to unmount test user')

        # Ensure that the user directory is not mounted
        if proxy.is_mounted(test_user):
          raise error.TestFail('Cryptohome mounted after unmount!')

        # Make sure that an incorrect password fails
        incorrect_password = 'this_is_an_incorrect_password'
        if proxy.mount(test_user, incorrect_password):
          raise error.TestFail('Cryptohome mounted with a bad password.')
        # Ensure that the user directory is not mounted
        if proxy.is_mounted(test_user):
          raise error.TestFail('Cryptohome mounted even though mount() failed')

        # Remove the test user account
        if not proxy.remove(test_user):
          raise error.TestFail('Cryptohome could not clean up vault')
