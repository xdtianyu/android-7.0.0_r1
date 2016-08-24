# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib.cros.tendo import buffet_tester

class buffet_IntermittentConnectivity(test.test):
    """Test that buffet reconnects if it loses connectivity."""
    version = 1

    def initialize(self):
        self._helper = buffet_tester.BuffetTester()


    def run_once(self):
        """Test entry point."""
        # Erase all buffet state and restart it pointing to our fake
        # server, register with the cloud and check we can poll for
        # commands.
        self._helper.restart_buffet(reset_state=True)
        self._helper.check_buffet_status_is(buffet_tester.STATUS_UNCONFIGURED)
        device_id = self._helper.register_with_server()
        self._helper.check_buffet_is_polling(device_id)

        # Now make fake_device_server fail all request from Buffet
        # with HTTP Error Code 500 (Internal Server Error) and check
        # that we transition to the CONNECTING state.
        self._helper._fail_control_client.start_failing_requests()
        self._helper.check_buffet_status_is(
                buffet_tester.STATUS_CONNECTING,
                expected_device_id=device_id,
                timeout_seconds=20)

        # Stop failing request from and check that we transition to
        # the CONNECTED state.
        self._helper._fail_control_client.stop_failing_requests()
        self._helper.check_buffet_status_is(
                buffet_tester.STATUS_CONNECTED,
                expected_device_id=device_id,
                timeout_seconds=20)
        self._helper.check_buffet_is_polling(device_id)


    def cleanup(self):
        self._helper.close()
