# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import pkcs11

class platform_Pkcs11ChangeAuthData(test.test):
    version = 1

    def run_once(self):
        pkcs11.setup_p11_test_token(True, 'auth1')
        pkcs11.load_p11_test_token('auth1')
        utils.system('p11_replay --inject --replay_wifi')
        # Change auth data while the token is not loaded.
        pkcs11.unload_p11_test_token()
        pkcs11.change_p11_test_token_auth_data('auth1', 'auth2')
        pkcs11.load_p11_test_token('auth2')
        result = utils.system('p11_replay --replay_wifi', ignore_status=True)
        if result != 0:
            raise error.TestFail('Change authorization data failed (1).')
        # Change auth data while the token is loaded.
        pkcs11.change_p11_test_token_auth_data('auth2', 'auth3')
        pkcs11.unload_p11_test_token()
        pkcs11.load_p11_test_token('auth3')
        result = utils.system('p11_replay --replay_wifi', ignore_status=True)
        if result != 0:
            raise error.TestFail('Change authorization data failed (2).')
        # Attempt change with incorrect current auth data.
        pkcs11.unload_p11_test_token()
        pkcs11.change_p11_test_token_auth_data('bad_auth', 'auth4')
        pkcs11.load_p11_test_token('auth3')
        result = utils.system('p11_replay --replay_wifi', ignore_status=True)
        if result != 0:
            raise error.TestFail('Change authorization data failed (3).')
        # Verify old auth data no longer works after change. This also verifies
        # recovery from bad auth data - expect a functional, empty token.
        pkcs11.unload_p11_test_token()
        pkcs11.change_p11_test_token_auth_data('auth3', 'auth5')
        pkcs11.load_p11_test_token('auth3')
        result = utils.system('p11_replay --replay_wifi', ignore_status=True)
        if result == 0:
            raise error.TestFail('Bad authorization data allowed (1).')
        utils.system('p11_replay --inject --replay_wifi')
        pkcs11.unload_p11_test_token()
        # Token should have been recreated with 'auth3'.
        pkcs11.load_p11_test_token('auth3')
        result = utils.system('p11_replay --replay_wifi', ignore_status=True)
        if result != 0:
            raise error.TestFail('Token not valid after recovery.')
        pkcs11.unload_p11_test_token()
        # Since token was recovered, previous correct auth should be rejected.
        pkcs11.load_p11_test_token('auth5')
        result = utils.system('p11_replay --replay_wifi', ignore_status=True)
        if result == 0:
            raise error.TestFail('Bad authorization data allowed (2).')
        pkcs11.unload_p11_test_token()
        pkcs11.cleanup_p11_test_token()
