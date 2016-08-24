# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import time

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import chrome
from autotest_lib.client.cros import pkcs11

class platform_Pkcs11InitOnLogin(test.test):
    """This test logs in and verifies that the TPM token is working."""
    version = 1

    def run_once(self):
        start_time = time.time()
        with chrome.Chrome() as cr:
            if not pkcs11.wait_for_pkcs11_token():
                raise error.TestFail('The PKCS #11 token is not available.')
            end_time = time.time()
            self.write_perf_keyval(
                { 'seconds_pkcs11_onlogin_init': end_time - start_time } )
            if not pkcs11.verify_pkcs11_initialized():
                raise error.TestFail('Initialized token failed checks!')
            if not pkcs11.inject_and_test_key():
                raise error.TestFail('Failed to inject a key.')
        # Login again with the same account.
        with chrome.Chrome(dont_override_profile=True) as cr:
            if not pkcs11.wait_for_pkcs11_token():
                raise error.TestFail(
                    'The PKCS #11 token is no longer available.')
            if not pkcs11.test_and_cleanup_key():
                raise error.TestFail('The PKCS #11 key is no longer valid.')

