# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import time

from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.networking.chrome_testing import test_utils

class ChromeNetworkProvider(object):
    """
    ChromeNetworkProvider handles all calls made to the connection
    manager by internally calling the Networking Private Extension API.

    For Example: Enable/Disable WiFi, Scan WiFi, Connect to WiFi that can
    be used to develop other tests.
    """

    WIFI_DEVICE = 'WiFi'
    CELLULAR = 'Cellular'
    SHORT_TIMEOUT = 3


    def __init__(self, cntc):
        """Initialization function for this class.

        @param cntc: Instance of type ChromeNetworkingTestContext.

        """
        self._chrome_testing = cntc


    def get_wifi_networks(self):
        """Get list of available wifi networks.

        @raises error.TestFail if no wifi networks are found.
        @return List of dictionaries containing wifi network information.

        """
        wifi_networks = self._chrome_testing.find_wifi_networks()
        if not wifi_networks:
            raise error.TestFail('No wifi networks found.')

        return wifi_networks


    def get_enabled_devices(self):
        """Get list of enabled network devices on the device.

        @return List of enabled network devices.

        """
        enabled_network_types = self._chrome_testing.call_test_function(
                test_utils.LONG_TIMEOUT,
                'getEnabledNetworkDevices')
        for key, value in enabled_network_types.items():
            if key == 'result':
                logging.info('Enabled Network Devices: %s', value)
                return value


    def disable_network_device(self, network):
        """Disable given network device.

        @param network: string name of the network device to be disabled.
                Options include 'WiFi', 'Cellular' and 'Ethernet'.

        """
        # Do ChromeOS browser session teardown/setup before disabling the
        # network device because chrome.networkingPrivate.disableNetworkType API
        # fails to disable the network device on subsequent calls if we do not
        # teardown and setup the browser session.
        self._chrome_testing.teardown()
        self._chrome_testing.setup()

        logging.info('Disabling: %s', network)
        disable_network_result = self._chrome_testing.call_test_function_async(
                'disableNetworkDevice',
                '"' + network + '"')


    def enable_network_device(self, network):
        """Enable given network device.

        @param network: string name of the network device to be enabled. Options
                include 'WiFi', 'Cellular' and 'Ethernet'.

        """
        logging.info('Enabling: %s', network)
        enable_network_result = self._chrome_testing.call_test_function_async(
                'enableNetworkDevice',
                '"' + network + '"')
        # Allow enough time for the DUT to fully transition into enabled state.
        time.sleep(self.SHORT_TIMEOUT)


    def scan_for_networks(self, timeout=SHORT_TIMEOUT):
        """Scan for all the available networks

        @param timeout int seconds to sleep while scanning for networks 

        """
        self._chrome_testing.call_test_function_async('requestNetworkScan')
        # Allow enough time for Chrome to scan and get all the network SSIDs.
        time.sleep(timeout)


    def connect_to_network(self, service_list):
       """Connects to the given network using networkingPrivate API.

       @param service_list: service list for the network to connect to.

       """
       connect_status = self._chrome_testing.call_test_function(
                            test_utils.LONG_TIMEOUT,
                            'connectToNetwork',
                            '"' + service_list['GUID'] +'"')

       if connect_status['error'] == 'connected':
           return
       elif connect_status['error'] == 'connecting':
           for retry in range(3):
               logging.debug('Just hold on for 10 seconds')
               time.sleep(10)
               if connect_status['error'] == 'connected':
                   return

       if connect_status['status'] == 'chrome-test-call-status-failure':
           raise error.TestFail(
                     'Could not connect to %s network. Error returned by '
                     'chrome.networkingPrivate.startConnect API: %s' %
                     (service_list['Name'], connect_status['error']))
