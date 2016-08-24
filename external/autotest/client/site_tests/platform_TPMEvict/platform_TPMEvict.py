# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.bin import test, utils
from autotest_lib.client.cros import pkcs11


class platform_TPMEvict(test.test):
    version = 1

    def run_once(self):
        pkcs11.setup_p11_test_token(True)
        pkcs11.load_p11_test_token()
        for i in range(30):
            utils.system('p11_replay --inject --replay_wifi')
        for i in range(30):
            utils.system('p11_replay --inject')
        utils.system('p11_replay --replay_wifi')
        utils.system('p11_replay --cleanup')
        pkcs11.cleanup_p11_test_token()
