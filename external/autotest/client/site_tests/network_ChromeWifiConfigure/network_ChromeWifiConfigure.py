# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.networking.chrome_testing \
        import chrome_networking_test_context as cntc
from autotest_lib.client.cros.networking.chrome_testing import test_utils

class network_ChromeWifiConfigure(test.test):
    """
    Tests wifi configuration using chrome.networkingPrivate.
    This test appears fairly trivial, but actually tests that Shill and Chrome
    are communicating successfully, and that Shill can successfully configure
    a WiFi network and retreive the properties of that network.

    """
    version = 1

    def _test_property(self, network, property_name, expected_value):
        value = test_utils.get_ui_property(network, property_name)
        if value != expected_value:
            raise error.TestFail('Expected value for "' + property_name +
                                 '" to be "' + str(expected_value) +
                                 '", found "' + str(value)) + '"'

    def _create_wifi(self, ssid, security):
        logging.info('create_wifi')
        shared = 'true'
        properties = {
            'Type': 'WiFi',
            'WiFi': {
                'SSID': ssid,
                'Security': security
                }
            }
        logging.info('Calling createNetwork')
        guid = test_utils.call_test_function_check_success(
                self._chrome_testing,
                'createNetwork',
                (shared, properties))
        logging.info(' guid: ' + guid)

        logging.info('Calling getNetworkInfo')
        network = test_utils.call_test_function_check_success(
                self._chrome_testing,
                'getNetworkInfo',
                ('"' + guid + '"',))
        logging.info(' result: ' + str(network))

        self._test_property(network, 'Type', 'WiFi')
        self._test_property(network, 'WiFi.Security', security)


    def _run_once_internal(self):
        logging.info('run_once_internal')
        self._create_wifi('test_wifi1', 'WEP-PSK')


    def run_once(self):
        with cntc.ChromeNetworkingTestContext() as testing_context:
            self._chrome_testing = testing_context
            self._run_once_internal()
