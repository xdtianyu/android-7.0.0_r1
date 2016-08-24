# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import time

from autotest_lib.client.common_lib import error
from autotest_lib.client.bin import test
from autotest_lib.client.cros.networking.chrome_testing \
        import chrome_networking_test_context as cntc
from autotest_lib.client.cros.networking.chrome_testing \
        import chrome_networking_test_api as cnta
from autotest_lib.client.common_lib.cros.network \
         import chrome_net_constants

class network_RoamWifiEndToEnd(test.test):
    """
    Tests the following with chrome.networkingPrivate APIs:
        1. Tests that the DUT can see and connect to the configured AP.
        2. Tests the DUT can Roam/Failover between networks with same SSID.

    """
    version = 1


    def _find_configured_network(self):
        """Check if the required network is in the networks_found list.

        @return Service list of the required network; None if network not found

        """
        networks = self.chrome_networking.get_wifi_networks()

        for service in networks:
            if service['Name'] == self.SSID:
                return service
        return None


    def _find_and_connect_to_configured_network(self):
        """Find all networks in range and connect to the required network."""

        network = self._find_configured_network()

        if network is None:
            raise error.TestFail('Network with ssid=%s is not found', self.SSID)
        self.chrome_networking.connect_to_network(network)


    def _verify_device_roamed(self):
        """Verify that the device has roamed between networks.

        @raises error.TestFail if connect to second AP has failed.

        """
        # Give the DUT some time to roam to the other AP and connect.
        time.sleep(self.chrome_networking.SHORT_TIMEOUT)
        required_network_service_list = self._find_configured_network()

        # Check if the network exists and it is in connected state.
        if required_network_service_list is None:
            raise error.TestFail('Network with ssid=%s is not found', self.SSID)

        if required_network_service_list['ConnectionState'] != 'Connected':
            raise error.TestFail(
                'DUT failed to roam/connect to the second network')

        if required_network_service_list['ConnectionState'] == 'Connected':
            logging.info('DUT successfully roamed to the other Open network')


    def run_once(self, ssid, test):
        """Run the test.

        @param ssid: SSID of the APs.
        @param test: Set by the server test control file depending on the test
                that is being run.

        """
        self.SSID = ssid

        with cntc.ChromeNetworkingTestContext() as testing_context:
            self.chrome_networking = cnta.ChromeNetworkProvider(testing_context)
            enabled_devices = self.chrome_networking.get_enabled_devices()
            if (self.chrome_networking.WIFI_DEVICE not in
                    enabled_devices):
                self.chrome_networking.enable_network_device(
                    self.chrome_networking.WIFI_DEVICE)

            self.chrome_networking.scan_for_networks()
            if test == chrome_net_constants.OPEN_CONNECT:
                self._find_and_connect_to_configured_network()
            elif test == chrome_net_constants.OPEN_ROAM:
                self._verify_device_roamed()
