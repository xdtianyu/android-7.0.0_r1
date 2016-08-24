# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import time
import urlparse

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import backchannel, network
from autotest_lib.client.cros.cellular import cell_tools
from autotest_lib.client.cros.networking import shill_context

# Import 'flimflam_test_path' first in order to import 'flimflam'.
# pylint: disable=W0611
from autotest_lib.client.cros import flimflam_test_path
# pylint: enable=W0611
import flimflam


# TODO(benchan): Use the log scopes defined in ShillProxy after
# migrating from FlimFlam to ShillProxy.
SHILL_LOG_SCOPES = 'dbus+device+dhcp+manager+portal+service+wimax'


class network_WiMaxSmoke(test.test):
    """Verifies that a WiMAX module can connect to a WiMAX network.

    The test attempts to connect to a WiMAX network. It assumes that a WiMAX
    module is plugged in to the DUT and a WiMAX network is available. It repeats
    the connect/disconnect sequence several times. Each time after connecting to
    the WiMAX network, it fetches some data from a URL to confirm the WiMAX
    connectivity.

    """
    version = 1

    def _connect_service(self):
        """Connects the WiMAX service under test.

        Raises:
            error.TestFail if it fails to connect the service before timeout.

        """
        logging.info('ConnectService: %s', self._service.object_path)

        service_properties = self._service.GetProperties()
        parameters = {
            'Type': 'wimax',
            'Name': str(service_properties['Name']),
            'NetworkId': str(service_properties['NetworkId']),
            'EAP.Identity': 'test',
            'EAP.Password': 'test',
        }
        logging.info('parameters : %s', parameters)
        self._flim.manager.ConfigureService(parameters)

        success, status = self._flim.ConnectService(
            service=self._service,
            config_timeout=self._connect_timeout)
        if not success:
            raise error.TestFail('Could not connect: %s.' % status)

        logging.info('Waiting for portal or online state.')
        portal_or_online_states = ['portal', 'online']
        state = self._flim.WaitForServiceState(
            service=self._service,
            expected_states=portal_or_online_states,
            timeout=self._connect_timeout,
            ignore_failure=True)[0]
        if not state in portal_or_online_states:
            raise error.TestFail('Still in state %s' % state)


    def _disconnect_service(self):
        """Disconnects the WiMAX service under test.

        Raises:
            error.TestFail if it fails to disconnect the service before
                timeout.

        """
        logging.info('DisonnectService: %s', self._service.object_path)

        success, status = self._flim.DisconnectService(
            service=self._service,
            wait_timeout=self._disconnect_timeout)
        if not success:
            raise error.TestFail('Could not disconnect: %s.' % status)


    def _test_connectivity(self):
        """Tests network connectivity over WiMAX.

        Test network connectivity over WiMAX as follows:
            - Connecting the WiMAX service
            - Fetching data from a URL
            - Disconnecting the WiMAX service

        Raises:
            error.TestFail if any of the steps above fails.

        """
        if self._sleep_kludge:
            logging.info('Sleeping for %.1f seconds', self._sleep_kludge)
            time.sleep(self._sleep_kludge)

        self._connect_service()

        device = self._flim.GetObjectInterface(
            'Device', self._service.GetProperties()['Device'])
        interface = device.GetProperties()['Interface']
        logging.info('Expected interface for %s: %s',
                     self._service.object_path, interface)
        network.CheckInterfaceForDestination(
            urlparse.urlparse(self._fetch_url_pattern).hostname,
            interface)

        fetch_time = network.FetchUrl(self._fetch_url_pattern,
                                      self._bytes_to_fetch,
                                      self._fetch_timeout)
        self.write_perf_keyval({
            'seconds_WiMAX_fetch_time': fetch_time,
            'bytes_WiMAX_bytes_received': self._bytes_to_fetch,
            'bits_second_WiMAX_speed': 8 * self._bytes_to_fetch / fetch_time
        })

        self._disconnect_service()


    def run_once(self, **kwargs):
        # Number of connectivity test runs.
        self._connect_count = kwargs.get('connect_count', 5)

        # Number of seconds to sleep between connect and disconnect operations.
        self._sleep_kludge = kwargs.get('sleep_kludge', 5)

        # URL pattern to fetch data from during each connectivity test run.
        self._fetch_url_pattern = \
            kwargs.get('fetch_url_pattern', network.FETCH_URL_PATTERN_FOR_TEST)

        # Timeout in seconds for connect and disconnect operations, and
        # fetching data from a URL.
        self._connect_timeout = kwargs.get('connect_timeout', 10)
        self._disconnect_timeout = kwargs.get('disconnect_timeout', 10)
        self._fetch_timeout = kwargs.get('fetch_timeout', 120)

        # Number of bytes to fetch during each connectivity test run.
        self._bytes_to_fetch = kwargs.get('bytes_to_fetch', 64 * 1024)

        with backchannel.Backchannel():
            with cell_tools.OtherDeviceShutdownContext('wimax'):
                # TODO(benchan): Replace FlimFlam with ShillProxy.
                self._flim = flimflam.FlimFlam()
                self._flim.SetDebugTags(SHILL_LOG_SCOPES)
                self._service = self._flim.FindWimaxService()
                if not self._service:
                    raise error.TestError('Could not find a WiMAX service.')

                with shill_context.ServiceAutoConnectContext(
                    self._flim.FindWimaxService, False):
                    self._disconnect_service()
                    for _ in xrange(self._connect_count):
                        self._test_connectivity()
