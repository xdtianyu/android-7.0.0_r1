# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import cryptohome, pkcs11


class platform_CryptohomeMigrateChapsTokenClient(test.test):
    """ This is a helper to platform_CryptohomeMigrateChapsToken
        It logs a test user in and either generates a chaps signing
        key or checks if a signing key was generated
    """
    version = 1


    def initialize(self):
        super(platform_CryptohomeMigrateChapsTokenClient, self).initialize()
        self._cryptohome_proxy = cryptohome.CryptohomeProxy()


    def run_once(self, generate_key=False):
        user = "user@test.com"
        password = "test_password"
        if generate_key:
            # We generate a chaps key tied to |user|.
            self._cryptohome_proxy.ensure_clean_cryptohome_for(user, password)
            result = pkcs11.generate_user_key()
            if not result:
                raise error.TestFail('Unable to generate key for ' + user)
        else:
            # Check if the chaps key previously generated is still present.
            # If the key is present, migration was successful, and chaps keys
            # weren't destroyed.
            result = self._cryptohome_proxy.mount(user, password)
            if not result:
                raise error.TestFail('Unable to remount users cryptohome')
            result = pkcs11.test_and_cleanup_key()
            if not result:
                raise error.TestFail('No Generated keys present for ' + user)
            self._cryptohome_proxy.remove(user)

