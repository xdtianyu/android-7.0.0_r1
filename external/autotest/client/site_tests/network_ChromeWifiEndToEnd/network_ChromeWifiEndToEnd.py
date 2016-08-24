# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.networking.chrome_testing \
        import chrome_networking_test_context as cntc
from autotest_lib.client.cros.networking.chrome_testing \
        import chrome_networking_test_api as cnta
from autotest_lib.client.cros.networking.chrome_testing import test_utils
from collections import namedtuple

NetworkInfo = namedtuple('NetworkInfo', 'name guid connectionState security')

class network_ChromeWifiEndToEnd(test.test):
    """
    Tests the following with chrome.networkingPrivate APIs:
        1. Tests that the configured wifi networks are seen by Chrome.
        2. Tests the transitioning between various available networks.
        3. Tests that the enabling and disabling WiFi works.
        4. Tests that the DUT autoconnects to previously connected WiFi network.

    """
    version = 1


    def _extract_wifi_network_info(self, networks_found):
        """Extract the needed information from the list of networks found
        via API.

        @param networks_found: Networks found via getVisibleNetworks api.
        @return Formated list of available wifi networks.

        """
        network_list = []

        for network in networks_found:
          network = NetworkInfo(name=network['Name'],
                                guid=network['GUID'],
                                connectionState=network['ConnectionState'],
                                security=network['WiFi']['Security'])
          network_list.append(network)

        return network_list


    def _wifi_network_comparison(
            self, configured_service_name_list, wifi_name_list):
        """Compare the known wifi SSID's against the SSID's returned via API.

        @param configured_service_name_list: Known SSID's that are configured
                by the network_WiFi_ChromeEndToEnd test.
        @param wifi_name_list: List of SSID's returned by the
                getVisibleNetworks API.
        @raises error.TestFail if network names do not match.

        """
        for name in configured_service_name_list:
            if name not in wifi_name_list:
                raise error.TestFail(
                    'Following network does not match: %s' % name)
        logging.info('Network names match!')


    def _enable_disable_network_check(
            self, original_enabled_networks, new_enabled_networks):
        """Tests enabling and disabling of WiFi.

        @param original_enabled_networks: List of network devices that were
                enabled when the test started.
        @param new_enabled_networks: List of network devices that are now
                now enabled.
        @raises error.TestFail if WiFi state is not toggled.

        """
        # Make sure we leave the WiFi network device in enabled state before
        # ending the test.
        self.chrome_net.enable_network_device(self.chrome_net.WIFI_DEVICE)

        if (self.chrome_net.WIFI_DEVICE in original_enabled_networks and
                self.chrome_net.WIFI_DEVICE in new_enabled_networks):
            raise error.TestFail('WiFi was not disabled.')
        if (self.chrome_net.WIFI_DEVICE not in original_enabled_networks
                and self.chrome_net.WIFI_DEVICE not in
                    new_enabled_networks):
            raise error.TestFail('WiFi was not enabled.')
        logging.info('Enabling / Disabling WiFi works!')


    def _connect_to_network(self, network):
        """Connects to the given network using networkingPrivate API.

        @param network: Namedtuple containing network attributes.

        """
        new_network_connect = self.chrome_net._chrome_testing.call_test_function(
                test_utils.LONG_TIMEOUT,
                'connectToNetwork',
                '"' + network.guid +'"')
        if (new_network_connect['status'] ==
                'chrome-test-call-status-failure'):
            raise error.TestFail(
                    'Could not connect to %s network. Error returned by '
                    'chrome.networkingPrivate.startConnect API: %s' %
                    (network.name, new_network_connect['error']))


    def _find_and_transition_wifi_networks_in_range(self):
        """Verify all WiFi networks in range are displayed."""
        known_service_names_in_wifi_cell = [self.SSID_1, self.SSID_2]
        networks_found_via_api = self.chrome_net.get_wifi_networks()
        network_list = self._extract_wifi_network_info(networks_found_via_api)
        logging.info('Networks found via API: %s', networks_found_via_api)

        wifi_names_found_via_api = []
        known_wifi_network_details = []

        for network in network_list:
            if network.name in known_service_names_in_wifi_cell:
                known_wifi_network_details.append(network)
            wifi_names_found_via_api.append(network.name)

        if self.TEST in ('all', 'findVerifyWiFiNetworks'):
            self._wifi_network_comparison(
                    known_service_names_in_wifi_cell, wifi_names_found_via_api)
        if self.TEST in ('all', 'transitionWiFiNetworks'):
            self._transition_wifi_networks(known_wifi_network_details)


    def _enable_disable_wifi(self):
        """Verify that the test is able to enable and disable WiFi."""
        original_enabled_networks = self.chrome_net.get_enabled_devices()
        if self.chrome_net.WIFI_DEVICE in original_enabled_networks:
            self.chrome_net.disable_network_device(self.chrome_net.WIFI_DEVICE)
        else:
            self.chrome_net.enable_network_device(self.chrome_net.WIFI_DEVICE)
        new_enabled_networks = self.chrome_net.get_enabled_devices()
        self._enable_disable_network_check(
                original_enabled_networks, new_enabled_networks)


    def _transition_wifi_networks(self, known_wifi_networks):
        """Verify that the test is able to transition between the two known
        wifi networks.

        @param known_wifi_networks: List of known wifi networks.
        @raises error.TestFail if device is not able to transition to a
                known wifi network.

        """
        if not known_wifi_networks:
            raise error.TestFail('No pre-configured network available for '
                                 'connection/transition.')

        for network in known_wifi_networks:
            self._connect_to_network(network)
            logging.info('Successfully transitioned to: %s', network.name)


    def _autoconnect_wifi(self):
        """Test and verify the device autoconnects to WiFi network.

        @raises error.TestFail if device is not able to autoconnect to a
                previously connected WiFi network.

        """
        networks = self._extract_wifi_network_info( \
                       self.chrome_net.get_wifi_networks())
        logging.info('Networks found before connection: %s', networks)
        network_to_connect = networks.pop()
        original_network_name = network_to_connect.name

        if network_to_connect.connectionState == 'NotConnected':
            self._connect_to_network(network_to_connect)
            logging.info('Connected to WiFi network: %s',
                         network_to_connect.name)

        self.chrome_net.disable_network_device(self.chrome_net.WIFI_DEVICE)
        self.chrome_net.enable_network_device(self.chrome_net.WIFI_DEVICE)
        self.chrome_net.scan_for_networks()

        networks = self._extract_wifi_network_info( \
                       self.chrome_net.get_wifi_networks())
        logging.info('Networks found after connection: %s', networks)
        network_to_connect = networks.pop()

        while network_to_connect.name != original_network_name:
            if not networks:
                raise error.TestFail('Previously connected WiFi network not '
                                     'found.')
            network_to_connect = networks.pop()

        if network_to_connect.connectionState == 'NotConnected':
            raise error.TestFail('Did not autoconnect to remembered network.')
        logging.info('Successfully autoconnected to remembered network.')


    def run_once(self, ssid_1, ssid_2, test):
        """Run the test.

        @param ssid_1: SSID of the first AP.
        @param ssid_2: SSID of the second AP.
        @param test: Set by the server test control file depending on the test
                that is being run.

        """
        self.SSID_1 = ssid_1
        self.SSID_2 = ssid_2
        self.TEST = test

        with cntc.ChromeNetworkingTestContext() as testing_context:
            self.chrome_net = cnta.ChromeNetworkProvider(testing_context)
            enabled_devices = self.chrome_net.get_enabled_devices()
            if (self.chrome_net.WIFI_DEVICE not in enabled_devices):
                self.chrome_net.enable_network_device(
                    self.chrome_net.WIFI_DEVICE)
            self.chrome_net.scan_for_networks()

            if test == 'all':
                self._find_and_transition_wifi_networks_in_range()
                self._enable_disable_wifi()
                self._autoconnect_wifi()
            elif test in ('findVerifyWiFiNetworks', 'transitionWiFiNetworks'):
                self._find_and_transition_wifi_networks_in_range()
            elif test == 'enableDisableWiFi':
                self._enable_disable_wifi()
            elif test == 'autoconnectWiFi':
                self._autoconnect_wifi()
