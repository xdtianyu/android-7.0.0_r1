# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib.cros.tendo import buffet_tester

class buffet_RefreshAccessToken(test.test):
    """Test that buffet can refresh its access token."""
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

        # Now invalidate buffet's current access token and check that
        # we can still poll for commands. This demonstrates that
        # buffet is able to get a new access token if the one that
        # it's been using has been revoked.
        self._helper._oauth_client.invalidate_all_access_tokens()
        self._helper.check_buffet_is_polling(device_id)
        self._helper.check_buffet_status_is(
                buffet_tester.STATUS_CONNECTED, expected_device_id=device_id)


    def cleanup(self):
        self._helper.close()
