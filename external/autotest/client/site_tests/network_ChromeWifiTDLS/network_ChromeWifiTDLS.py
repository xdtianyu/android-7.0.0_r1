# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.networking.chrome_testing \
        import chrome_networking_test_context as cntc
from autotest_lib.client.cros.networking.chrome_testing import test_utils

class network_ChromeWifiTDLS(test.test):
    """
    Tests that Shill responds to a Device.PerformTDLSOperation call via the
    networikingPrivate API. It does not assume that configuration will actually
    succeed (since that will depend on the environment), just that the call
    itself succeeds and the callback is invoked.
    """
    version = 1

    def _enable_tdls(self, ip_address):
        logging.info('enable_tdls')
        enable = 'true'
        result = test_utils.call_test_function_check_success(
            self._chrome_testing,
            'setWifiTDLSEnabledState',
            ('"' + ip_address + '"', enable))
        logging.info('tdls result: ' + result)

        # Look for a valid result.
        if (result != 'Connected' and result != 'Disabled' and
            result != 'Disconnected' and result != 'Nonexistant' and
            result != 'Unknown'):
            raise error.TestFail(
                'Unexpected result for setWifiTDLSEnabledState: ' + result)


    def _run_once_internal(self):
        logging.info('run_once_internal')
        self._enable_tdls('aa:bb:cc:dd:ee:ff')


    def run_once(self):
        with cntc.ChromeNetworkingTestContext() as testing_context:
            self._chrome_testing = testing_context
            self._run_once_internal()
