# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import dbus
import logging
import tempfile
import time

from autotest_lib.client.bin import test
from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros.network \
         import wifi_rack_constants as constants
from autotest_lib.client.cros.networking import wifi_proxy
from autotest_lib.client.cros.networking.chrome_testing \
        import chrome_networking_test_api as cnta
from autotest_lib.client.cros.networking.chrome_testing \
        import chrome_networking_test_context as cntc


class network_RackWiFiConnect(test.test):
    """Client test to connect to various network services on WiFi rack.

    After connection, we assert access to pages only accessible through the
    connected network.

    """
    version = 1


    def _assert_access(self, test):
        """Asset user can access page.

        Verification URLs are either pages on WiFi rack's Apache server or
        general Internet.

        @param test string - testname of NetworkServices namedtuple

        @return boolean - True if able to access, False otherwise

        """
        for service_test in constants.NETWORK_SERVICES_TESTS:
            if test == service_test.testname:
                url, pattern = service_test.url, service_test.pattern
                break

        # Since this test runs OTA, allow 15 seconds of leeway
        time.sleep(15)

        wget_cmd = 'wget -O /tmp/wget.log %s' % url
        for retry in range(3):
            exit_status = utils.system(wget_cmd, ignore_status=True)
            if not exit_status:
                logging.debug('Able to wget URL.')
                break
            logging.error('Could not wget URL; trying again.')

        grep_url_cmd = 'cat /tmp/wget.log | grep %s' % pattern
        output_status = utils.system(grep_url_cmd, ignore_status=True)
        if output_status:
            logging.debug('Unable to access correct URL for %s',
                          service_test.testname)
            return False
        return True


    def _connect(self, ssid, uname):
        """Connect to particular network and assert access to page.

        @param ssid string - predefined SSID from user's preferred networks
        @param uname string - predefined username of managed user

        @return boolean - True if able to connect, False otherwise

        """
        start_time = time.time()
        with cntc.ChromeNetworkingTestContext(username=uname,
                  password=constants.PASSWORD) as testing_context:
            net_provider = cnta.ChromeNetworkProvider(testing_context)
            enabled_devices = net_provider.get_enabled_devices()
            if net_provider.WIFI_DEVICE not in enabled_devices:
                net_provider.enable_network_device(net_provider.WIFI_DEVICE)
            logging.info('Scanning for networks')
            connect_to_service = None
            while time.time() - start_time < constants.SCAN_RETRY_TIMEOUT:
                net_provider.scan_for_networks(timeout=20)
                logging.info('Attempting to connect to %s', ssid)
                networks = net_provider.get_wifi_networks()
                for service in networks:
                    if service['Name'] == ssid:
                        connect_to_service = service
                if not connect_to_service:
                    logging.error('Unable to find %s', ssid)
                    continue
                try:
                    net_provider.connect_to_network(connect_to_service)
                    logging.info('Successfully connected to network %s', ssid)
                    return True
                except error.TestFail as e:
                    logging.error('Unable to connect to %s', ssid)
                    continue
            return False


    def _connect_and_assert(self, test, ssid, user):
        """Verify connect and assert and write results to results/.

        @param test string - testname of NetworkServices namedtuple
        @param ssid string - predefined SSID from user's preferred networks
        @param user string - predefined username of managed user

        """
        tf = tempfile.NamedTemporaryFile(suffix='.txt',
                                         prefix='connect_%s_' % test,
                                         dir=self.resultsdir,
                                         delete=False)
        with tf as results:
            if not self._connect(ssid, user):
                results.write('%s FAILED to connect to SSID\n\n' % test)
            elif not self._assert_access(test):
                results.write('%s FAILED to access\n\n' % test)
            else:
                results.write('%s passed\n\n' % test)


    def _to_wifi(self, proxy):
        """Set service order to WiFi before Ethernet.

        @param proxy WiFi Proxy object

        """
        logging.info('Setting order to WiFi, prioritized over Ethernet.')
        proxy.manager.SetServiceOrder(dbus.String('wifi,ethernet'))


    def _to_ethernet(self, proxy):
        """Set service order to default Ethernet before WiFi

        @param proxy WiFi Proxy object

        """
        logging.info('Setting back to default service order.')
        proxy.manager.SetServiceOrder(dbus.String('ethernet,wifi'))


    def run_once(self, test):
        """Run the test.

        @param test string - Set by the client test control file

        """
        client_proxy = wifi_proxy.WifiProxy()
        if test is not 'all':
            logging.info('Running an individual control file.')
            self._to_wifi(client_proxy)
            for service_test in constants.NETWORK_SERVICES_TESTS:
                if service_test.testname == test:
                    self._connect_and_assert(service_test.testname,
                                             service_test.ssid,
                                             service_test.user)
                    self._to_ethernet(client_proxy)
                    return
        for service_test in constants.NETWORK_SERVICES_TESTS:
            logging.info('==== Running current test %s ====',
                         service_test.testname)
            self._to_wifi(client_proxy)
            self._connect_and_assert(service_test.testname,
                                     service_test.ssid,
                                     service_test.user)
            self._to_ethernet(client_proxy)

        # Ensure DUT returns to normal service state
        self._to_ethernet(client_proxy)
