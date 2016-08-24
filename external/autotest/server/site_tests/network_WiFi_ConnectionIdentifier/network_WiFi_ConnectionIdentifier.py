# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import time

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros.network import xmlrpc_datatypes
from autotest_lib.server.cros.network import hostap_config
from autotest_lib.server.cros.network import wifi_cell_test_base


class network_WiFi_ConnectionIdentifier(wifi_cell_test_base.WiFiCellTestBase):
    """Test for verifying connection identifier."""
    version = 1

    CONNECTION_ID_TIMEOUT_SECS = 10
    SERVICE_PROPERTY_CONNECTION_ID = 'ConnectionId'

    def _get_service_connection_id(self, ssid):
        """Get the connection ID for a service.

        Polls a service's properties until ConnectionId becomes non-zero,
        or a timeout occurs.

        @param ssid: SSID of the service of interest.
        @raise TestError if a timeout occurs.
        @return ConnectionId of the current service.
        """
        start_time = time.time()
        while time.time() - start_time < self.CONNECTION_ID_TIMEOUT_SECS:
            properties = self.context.client.shill.get_service_properties(ssid)
            logging.debug('Service properties are: %s', properties)
            connection_id = properties[self.SERVICE_PROPERTY_CONNECTION_ID]
            if connection_id != 0:
                return connection_id
            time.sleep(1)
        raise error.TestFail('ConnectionId remained zero')


    def _connect(self, ssid, expected_connection_id=None):
        """Connect to an AP, and verify connection ID if it is specified.

        @param ssid: SSID of the AP.
        @param expected_connection_id: Expected connection ID.
        @return ConnectionId of the new connection.
        """
        client_conf = xmlrpc_datatypes.AssociationParameters(ssid)
        self.context.assert_connect_wifi(client_conf)
        connection_id = self._get_service_connection_id(ssid)
        if (expected_connection_id is not None and
                expected_connection_id != connection_id):
            raise error.TestFail(
              'Expected connection ID %s, but got %s' % (
                expected_connection_id, connection_id))
        return connection_id


    def run_once(self):
        """Test to verify connection id, which depends only on the network
        (gateway) that the AP is connected to."""

        # Configure two APs which will be automatically assigned different
        # SSIDs. Each AP instance is connected to specific gateway.
        router_conf = hostap_config.HostapConfig(channel=6);
        self.context.configure(router_conf)
        self.context.configure(router_conf, multi_interface=True)
        ssid0 = self.context.router.get_ssid(instance=0)
        ssid1 = self.context.router.get_ssid(instance=1)

        # Connect to both APs and save the connection ID for both connections.
        # Verify the connection ID is different for the two connections.
        connection_id0 = self._connect(ssid0)
        connection_id1 = self._connect(ssid1)
        if connection_id0 == connection_id1:
            raise error.TestFail('Connection ID should be different for two '
                                 'different networks')
        self.context.router.deconfig()

        # Reconfigure the router with different SSIDs, and verify the
        # connection ID sticks with AP instance regardless of the SSID.
        self.context.configure(router_conf)
        self.context.configure(router_conf, multi_interface=True)

        # Verify SSID is different
        if (self.context.router.get_ssid(instance=0) == ssid0 or
            self.context.router.get_ssid(instance=1) == ssid1):
            raise error.TestError('SSID should different from previous '
                                  'configuration')

        # Connect and verify connection ID stays the same for the same
        # AP instance.
        self._connect(self.context.router.get_ssid(instance=0),
                      expected_connection_id=connection_id0)
        self._connect(self.context.router.get_ssid(instance=1),
                      expected_connection_id=connection_id1)
