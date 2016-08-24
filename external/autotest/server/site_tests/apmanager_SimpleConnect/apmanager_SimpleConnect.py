# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros.network import apmanager_constants
from autotest_lib.client.common_lib.cros.network import xmlrpc_datatypes
from autotest_lib.server.cros.network import apmanager_service_provider
from autotest_lib.server.cros.network import wifi_cell_test_base


class apmanager_SimpleConnect(wifi_cell_test_base.WiFiCellTestBase):
    """Test that the DUT can connect to an AP created by apmanager."""
    version = 1

    XMLRPC_BRINGUP_TIMEOUT_SECONDS = 60

    def parse_additional_arguments(self, commandline_args, additional_params):
        """Hook into super class to take control files parameters.

        @param commandline_args dict of parsed parameters from the autotest.
        @param additional_params dict of AP configuration parameters.

        """
        self._configurations = additional_params


    def run_once(self):
        """Sets up a router, connects to it, pings it."""
        # Setup bridge mode test environments if AP is configured to operate in
        # bridge mode.
        if (apmanager_constants.CONFIG_OPERATION_MODE in self._configurations
            and self._configurations[apmanager_constants.CONFIG_OPERATION_MODE]
                    == apmanager_constants.OPERATION_MODE_BRIDGE):
            # Setup DHCP server on the other side of the bridge.
            self.context.router.setup_bridge_mode_dhcp_server()
            self._configurations[apmanager_constants.CONFIG_BRIDGE_INTERFACE] =\
                    self.context.router.get_bridge_interface()

        ssid = self.context.router.build_unique_ssid()
        self._configurations[apmanager_constants.CONFIG_SSID] = ssid
        with apmanager_service_provider.ApmanagerServiceProvider(
                self.context.router, self._configurations):
            assoc_params = xmlrpc_datatypes.AssociationParameters()
            assoc_params.ssid = ssid
            self.context.assert_connect_wifi(assoc_params)
            self.context.assert_ping_from_server()
        # AP is terminated, wait for client to become disconnected.
        success, state, elapsed_seconds = \
                self.context.client.wait_for_service_states(
                        ssid, ( 'idle', ), 30)
        if not success:
            raise error.TestFail('Failed to disconnect from %s after AP was '
                                 'terminated for %f seconds (state=%s)' %
                                 (ssid, elapsed_seconds, state))
