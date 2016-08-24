# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.common_lib.cros.network import xmlrpc_datatypes
from autotest_lib.server import site_linux_system
from autotest_lib.server.cros.network import hostap_config
from autotest_lib.server.cros.network import wifi_cell_test_base


class network_WiFi_CSADisconnect(wifi_cell_test_base.WiFiCellTestBase):
    """Test that verifies the client's MAC 80211 queues are not stuck when
    disconnecting immediately after receiving a CSA (Channel Switch
    Announcement) message. Refer to "crbug.com/408370" for more information."""
    version = 1


    def _connect_to_ap(self, channel):
        """Configure an AP and instruct client to connect to it with
        autoconnect disabled.

        @param channel int Channel to configure AP in.

        """
        self.context.configure(hostap_config.HostapConfig(
                channel=channel,
                mode=hostap_config.HostapConfig.MODE_11N_MIXED))
        assoc_params = xmlrpc_datatypes.AssociationParameters()
        assoc_params.ssid = self.context.router.get_ssid()
        assoc_params.autoconnect = False
        self.context.client.shill.configure_wifi_service(assoc_params)
        self.context.assert_connect_wifi(assoc_params)


    def _csa_test(self, router_initiated_disconnect):
        """Perform channel switch, and initiate disconnect immediately, then
        verify wifi connection still works, hence the 80211 queues are not
        stuck.

        @param router_initiated_disconnected bool indicating the initiator of
            the disconnect.

        """
        # Run it multiple times since the client might be in power-save,
        # we are not guaranteed it will hear this message the first time
        # around. Alternate the AP channel with the CSA announced channel to
        # work around with drivers (Marvell 8897) that disallow reconnecting
        # immediately to the same AP on the same channel after CSA to a
        # different channel.
        for attempt in range(5):
            self._connect_to_ap(self._primary_channel)
            self.context.router.send_management_frame_on_ap(
                'channel_switch', self._alternate_channel)
            if router_initiated_disconnect:
                self.context.router.deauth_client(self.context.client.wifi_mac)
            else:
                self.context.client.shill.disconnect(
                        self.context.router.get_ssid())

            # Wait for client to be disconnected.
            success, state, elapsed_seconds = \
                    self.context.client.wait_for_service_states(
                            self.context.router.get_ssid(), ('idle'), 30)

            # Swap primary_channel with alternate channel so we don't configure
            # AP using same channel in back-to-back runs.
            tmp = self._alternate_channel
            self._alternate_channel = self._primary_channel
            self._primary_channel = tmp


    def parse_additional_arguments(self, commandline_args, additional_params):
        """Hook into super class to take control files parameters.

        @param commandline_args dict of parsed parameters from the autotest.
        @param additional_params list of dicts describing router configs.

        """
        self._configurations = additional_params


    def run_once(self):
        """Verify that wifi connectivity still works when disconnecting
        right after channel switch."""

        for self._primary_channel, self._alternate_channel in \
                self._configurations:
            self.context.router.require_capabilities(
                  [site_linux_system.LinuxSystem.
                          CAPABILITY_SEND_MANAGEMENT_FRAME])
            # Test both router initiated and client initiated disconnect after
            # channel switch announcement.
            self._csa_test(True)
            self._csa_test(False)

            self.context.router.deconfig()
