# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros.network import xmlrpc_datatypes
from autotest_lib.server.cros.network import hostap_config
from autotest_lib.server.cros.network import wifi_cell_test_base


class network_WiFi_DisableEnable(wifi_cell_test_base.WiFiCellTestBase):
    """Tests that disabling an enabling WiFi re-connects the system.

    This test run seeks to associate the DUT with an AP, then toggle
    the "enable" flag on the WiFi device.  This should disconnect and
    reconnect the device.

    """

    version = 1

    def run_once(self):
        """Test body."""
        # Configure the AP.
        frequency = 2412
        self.context.configure(hostap_config.HostapConfig(frequency=frequency))
        router_ssid = self.context.router.get_ssid()

        # Connect to the AP.
        self.context.assert_connect_wifi(
                xmlrpc_datatypes.AssociationParameters(ssid=router_ssid))

        # Disable the interface only long enough that we're sure we have
        # disconnected.
        interface = self.context.client.wifi_if
        client = self.context.client
        with InterfaceDisableContext(client, interface) as idc:
            success, state, elapsed_seconds = client.wait_for_service_states(
                    router_ssid, ( 'idle', ), 3)
            # We should either be in the 'idle' state or not even know about
            # this service state anymore.  The latter is more likely since
            # the AP's service should lose visibility when the device is
            # disabled.
            if not success and state != 'unknown':
                raise error.TestFail(
                        'Failed to disconnect from "%s" after interface was '
                        'disabled for %f seconds (state=%s)' %
                        (router_ssid, elapsed_seconds, state))

        # Expect that the DUT will re-connect to the AP.
        self.context.wait_for_connection(router_ssid, frequency);
        self.context.router.deconfig()


class InterfaceDisableContext(object):
    """Context that encapsulates disabling of a device.

    This context ensures that if the test fails while the device is disabled
    we will attempt to re-enable it before our test exits.

    """

    def __init__(self, client, interface):
        self._client = client
        self._interface = interface


    def __enter__(self):
        self._client.set_device_enabled(self._interface, False)


    def __exit__(self, exception, value, traceback):
        self._client.set_device_enabled(self._interface, True)
