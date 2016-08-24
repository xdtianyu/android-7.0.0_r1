# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import cryptohome, pkcs11

class platform_CryptohomeKeyEviction(test.test):
    """Ensure that the cryptohome properly manages key eviction from the tpm.
       This test verifies this behaviour by creating 30 keys using chaps,
       and then remounting a user's cryptohome. Mount requires use of the
       user's cryptohome key, and thus the mount only succeeds if the
       cryptohome key was properly evicted and reloaded into the TPM.
    """
    version = 1


    def initialize(self):
        super(platform_CryptohomeKeyEviction, self).initialize()
        self._cryptohome_proxy = cryptohome.CryptohomeProxy()


    def run_once(self):
        self.user = 'first_user@nowhere.com'
        password = 'test_password'
        self._cryptohome_proxy.ensure_clean_cryptohome_for(self.user, password)

        # First we inject 30 tokens into chaps. This forces the cryptohome
        # key to get evicted.
        for i in range(30):
            pkcs11.inject_and_test_key()

        # Then we get a user to remount his cryptohome. This process uses
        # the cryptohome key, and if the user was able to login, the
        # cryptohome key was correctly reloaded.
        self._cryptohome_proxy.unmount(self.user)
        if not self._cryptohome_proxy.mount(self.user, password, create=True):
          raise error.TestFail('Failed to remount user\'s cryptohome')


    def cleanup(self):
        self._cryptohome_proxy.remove(self.user)
