# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import random, shutil
from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import pkcs11

class platform_Pkcs11Events(test.test):
    version = 1

    def run_once(self, num_tokens, num_events):
        # Setup some token directories.
        token_list = ['/tmp/chaps%d' % x for x in range(num_tokens)]
        pkcs11.setup_p11_test_token(True)
        shutil.rmtree('%s/database' % pkcs11.TMP_CHAPS_DIR, ignore_errors=True)
        for token in token_list:
            shutil.rmtree(token, ignore_errors=True)
            pkcs11.copytree_with_ownership(pkcs11.TMP_CHAPS_DIR, token)

        # Setup a key on each token.
        for token in token_list:
            utils.system('chaps_client --load --path=%s --auth=%s' %
                         (token, token))
            utils.system('p11_replay --inject')
            utils.system('chaps_client --unload --path=%s' % token)

        # Follow a login by an immediate logout.
        for token in token_list:
            utils.system('chaps_client --load --path=%s --auth=%s' %
                         (token, token))
        for token in token_list:
            utils.system('chaps_client --unload --path=%s' % token)

        # Hit the tokens with a bunch of random login / logout events.
        for i in range(num_events):
            token = random.choice(token_list)
            event = random.choice(['login', 'logout'])
            if event == 'login':
              utils.system('chaps_client --load --path=%s --auth=%s' %
                           (token, token))
              # Note: This won't necessarily test the token we just loaded but
              # we do know there should be at least one token available.
              result = utils.system('p11_replay --replay_wifi',
                                    ignore_status=True)
              if result != 0:
                  raise error.TestFail('At least one token is not functional.')
            else:
              utils.system('chaps_client --unload --path=%s' % token)

        # See if each token is still functional.
        for token in token_list:
            utils.system('chaps_client --unload --path=%s' % token)
        for token in token_list:
            utils.system('chaps_client --load --path=%s --auth=%s' %
                         (token, token))
            result = utils.system('p11_replay --replay_wifi',
                                  ignore_status=True)
            if result != 0:
                raise error.TestFail('Token is not functional: %s' % token)
            utils.system('chaps_client --unload --path=%s' % token)
            shutil.rmtree(token, ignore_errors=True)

        pkcs11.cleanup_p11_test_token()
