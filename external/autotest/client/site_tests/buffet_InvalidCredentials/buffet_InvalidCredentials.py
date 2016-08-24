# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib.cros.tendo import buffet_tester

class buffet_InvalidCredentials(test.test):
    """Test that buffet transitions properly if refresh token is revoked."""
    version = 1

    def initialize(self):
        self._helper = buffet_tester.BuffetTester()


    def run_once(self):
        # Erase all buffet state and restart it pointing to our fake
        # server, register with the cloud and check we can poll for
        # commands.
        self._helper.restart_buffet(reset_state=True)
        self._helper.check_buffet_status_is(buffet_tester.STATUS_UNCONFIGURED)
        device_id = self._helper.register_with_server()
        self._helper.check_buffet_is_polling(device_id)

        # Now invalidate buffet's access and refresh token and check
        # that buffet transitions to the invalid_credentials state and clears
        # the stored credentials/device_id.
        self._helper._oauth_client.invalidate_all_refresh_tokens()
        self._helper._oauth_client.invalidate_all_access_tokens()
        self._helper.check_buffet_status_is(
                buffet_tester.STATUS_INVALID_CREDENTIALS,
                expected_device_id=device_id,
                timeout_seconds=20)


    def cleanup(self):
        self._helper.close()
