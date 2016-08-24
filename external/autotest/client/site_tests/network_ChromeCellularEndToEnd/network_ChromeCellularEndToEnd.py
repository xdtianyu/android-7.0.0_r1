# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, json

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import sys_power
from autotest_lib.client.cros.networking.chrome_testing \
        import chrome_networking_test_context as cntc
from autotest_lib.client.cros.networking.chrome_testing \
        import chrome_networking_test_api as cnta
from autotest_lib.client.cros.networking.chrome_testing import test_utils
from collections import namedtuple

NetworkInfo = namedtuple('NetworkInfo', ['name', 'guid', 'connectionState',
                                         'networkType'])

class network_ChromeCellularEndToEnd(test.test):
    """
    Tests the following UI functionality with chrome.networkingPrivate APIs:
        1. Tests that the device auto connects to cellular network.
        3. Tests that the device prefers ethernet over cellular network.
        4. Tests that the enabling and disabling of cellular modem works.

    """
    version = 1


    def _extract_network_info(self, networks_found):
        """Extract the needed information from the list of networks.

        @param networks_found: Networks found via api.
        @return Formatted list of available cellular networks.

        """
        formatted_network_list = []

        for network in networks_found:
          network = NetworkInfo(name=network['Name'],
                  guid=network['GUID'],
                  connectionState=network.get('ConnectionState', 'none'),
                  networkType=network['Type'])
          formatted_network_list.append(network)

        return formatted_network_list


    def _set_autoconnect(self, service, value=True):
        """Turn on autoconnect for cellular network.

        @param service: Cellular service dictionary
        @value: Set / unset autoconnect

        """
        logging.debug('_set_autoconnect')
        properties = json.dumps({'Cellular': {'AutoConnect': value}})
        set_properties = test_utils.call_test_function_check_success(
                self._chrome_testing,
                'setProperties',
                ('"' + service['GUID'] + '"', properties))
        self.chrome_net.scan_for_networks()


    def _is_cellular_network_connected(self, service):
        """Check if device is connected to cellular network.

        @param service: Cellular service dict
        @return True if connected to cellular, else False

        """
        network_properties = self.chrome_net._chrome_testing.call_test_function(
                test_utils.LONG_TIMEOUT,
                'getNetworkInfo',
                ('"' + service['GUID'] + '"'))

        logging.debug('Network properties: %s', network_properties)

        if network_properties['status'] == 'chrome-test-call-status-failure':
            raise error.TestFail('getNetworkInfo did not return with status '
                                 'SUCCESS: %s' % network_properties['error'])

        if network_properties['result']['ConnectionState'] == 'Connected':
            return True

        return False


    def _cellular_service(self):
        """Find cellular service.

        @return Cellular service dict

        """
        cell_networks = self.chrome_net._chrome_testing.find_cellular_networks()
        for service in cell_networks:
            if service['GUID']:
                return service

        return None


    def _find_cellular_service(self):
        """Find and return cellular service if available.

        @return Cellular service

        """
        utils.poll_for_condition(lambda: self._cellular_service() is not None,
                                 exception=error.TestFail('No cell service.'),
                                 sleep_interval=1,
                                 timeout=60)
        return self._cellular_service()


    def _connect_to_cellular_network(self, service):
        """Connect to cellular network.

        @param service: Cellular service dict

        """
        logging.debug('_connect_to_cellular_network')
        if service is None:
            raise error.TestFail('GUID not available for cellular network.')
        self.chrome_net.connect_to_network(service)
        self.chrome_net.scan_for_networks()


    def _autoconnect_cellular(self):
        """Verify that the DUT is able to autoconnect to cellular network."""
        logging.debug('_autoconnect_cellular')
        service = self._find_cellular_service()
        logging.debug('Cellular service: %s', service)

        if service['ConnectionState'] == 'NotConnected':
            self._connect_to_cellular_network(service)

        self._set_autoconnect(service)

        logging.debug('Suspend and resume device')
        sys_power.do_suspend(20)
        service = self._find_cellular_service()

        utils.poll_for_condition(
                lambda: self._is_cellular_network_connected(service),
                exception=error.TestFail('Network not connected after suspend '
                                         'and resume.'),
                sleep_interval=1,
                timeout=60)
        logging.debug('Autoconnect works after suspend/resume.')


    def _get_networks(self, network_type='All'):
        """Get available networks with getNetworks api.

        @param network_type: Type of network, defaults to All
        @return List of networks found

        """
        logging.debug('_get_networks')
        properties = json.dumps({'networkType': network_type,
                                 'visible': True,
                                 'limit': 2})
        network_list = self.chrome_net._chrome_testing.call_test_function(
                test_utils.LONG_TIMEOUT,
                'getNetworks',
                (properties))
        return network_list


    def _ethernet_preferred_over_cellular(self):
        """Verify that the DUT prefers ethernet over cellular connection."""
        logging.debug('_ethernet_preferred_over_cellular')
        self.chrome_net.disable_network_device(self.chrome_net.WIFI_DEVICE)
        network_list = self._get_networks()

        if not len(network_list['result']) > 1:
            logging.debug('Available networks: %s', network_list)
            raise error.TestFail('Not enough networks available to check '
                                 'network preference. Need minimum 2 networks '
                                 'to do a successfull comparision.')

        formatted_network_list = self._extract_network_info(
                network_list['result'])

        logging.debug('Available network list: %s', formatted_network_list)

        if not formatted_network_list[0].networkType == 'Ethernet':
            raise error.TestFail('Ethernet is not preferred.')

        if not formatted_network_list[1].networkType == 'Cellular':
            raise error.TestFail('Cellular is not available to determine '
                                 'network preference.')

        if (not formatted_network_list[0].connectionState == 'Connected' or
            not formatted_network_list[1].connectionState == 'Connected'):
            raise error.TestFail('Ethernet and Cellular should both be '
                                 'connected to successfully determine '
                                 'network preference.')

        logging.debug('Ethernet is preferred over cellular.')


    def _enable_disable_network_check(
            self, original_enabled_networks, new_enabled_networks):
        """Tests enabling and disabling of Cellular.

        @param original_enabled_networks: Original list of network devices that
                were enabled when the test started.
        @param new_enabled_networks: New list of network devices that are
                enabled now.
        @raises error.TestFail if Cellular state is not toggled.

        """
        # Make sure we leave the Cellular modem in enabled state before
        # ending the test.
        logging.debug('_enable_disable_network_check')
        self.chrome_net.enable_network_device(self.chrome_net.CELLULAR)

        if self.chrome_net.CELLULAR in original_enabled_networks:
            if self.chrome_net.CELLULAR in new_enabled_networks:
                raise error.TestFail('Cellular was not disabled.')
            elif self.chrome_net.CELLULAR not in new_enabled_networks:
                logging.info('Cellular was successfully disabled.')

        if self.chrome_net.CELLULAR not in original_enabled_networks:
            if self.chrome_net.CELLULAR not in new_enabled_networks:
                raise error.TestFail('Cellular was not enabled.')
            elif self.chrome_net.CELLULAR in new_enabled_networks:
                logging.info('Cellular was successfully enabled.')


    def _enable_disable_cellular(self):
        """Verify that the test is able to enable and disable Cellular."""
        logging.debug('_enable_disable_cellular')
        original_enabled_networks = self.chrome_net.get_enabled_devices()
        if self.chrome_net.CELLULAR in original_enabled_networks:
            self.chrome_net.disable_network_device(self.chrome_net.CELLULAR)
        else:
            self.chrome_net.enable_network_device(self.chrome_net.CELLULAR)
        new_enabled_networks = self.chrome_net.get_enabled_devices()
        self._enable_disable_network_check(
                original_enabled_networks, new_enabled_networks)


    def run_once(self, test):
        """Runs the test.

        @param test: Set by the server test control file depending on the test
                that is being run.

        """
        with cntc.ChromeNetworkingTestContext() as testing_context:
            self._chrome_testing = testing_context
            self.chrome_net = cnta.ChromeNetworkProvider(testing_context)
            enabled_devices = self.chrome_net.get_enabled_devices()

            logging.debug('Enabled devices: %s', enabled_devices)
            if (self.chrome_net.CELLULAR not in enabled_devices):
                self.chrome_net.enable_network_device(
                    self.chrome_net.CELLULAR)
            self.chrome_net.scan_for_networks()

            if test == 'autoconnectCellular':
                self._autoconnect_cellular()
            elif test == 'ethernetPreferred':
                self._ethernet_preferred_over_cellular()
            elif test == 'enableDisableCellular':
                self._enable_disable_cellular()
