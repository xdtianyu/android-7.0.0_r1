# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros.network import iw_runner
from autotest_lib.client.common_lib.cros.network import xmlrpc_datatypes
from autotest_lib.server.cros.network import hostap_config
from autotest_lib.server.cros.network import wifi_cell_test_base

class network_WiFi_ChannelHop(wifi_cell_test_base.WiFiCellTestBase):
    """Tests roaming when an AP changes channels on an SSID."""

    version = 1
    ORIGINAL_FREQUENCY = 2412
    ORIGINAL_BSSID = "00:01:02:03:04:05"
    TEST_SSID="HowHeGotInMyPajamasIllNeverKnow"

    def run_once(self):
        """Test body."""
        freq = network_WiFi_ChannelHop.ORIGINAL_FREQUENCY
        ap_config = hostap_config.HostapConfig(
                ssid=network_WiFi_ChannelHop.TEST_SSID,
                frequency=freq,
                mode=hostap_config.HostapConfig.MODE_11B,
                bssid=network_WiFi_ChannelHop.ORIGINAL_BSSID)
        self.context.configure(ap_config)
        assoc_params = xmlrpc_datatypes.AssociationParameters(
                ssid=self.context.router.get_ssid())
        self.context.assert_connect_wifi(assoc_params)

        self.context.assert_ping_from_dut()
        self.context.client.check_iw_link_value(
                iw_runner.IW_LINK_KEY_FREQUENCY,
                freq)
        self.context.router.deconfig()

        # This checks both channel jumping on the same BSSID and channel
        # jumping between BSSIDs, all inside the same SSID.
        for freq, bssid in ((2437, network_WiFi_ChannelHop.ORIGINAL_BSSID),
                            (2462, network_WiFi_ChannelHop.ORIGINAL_BSSID),
                            (2422, "06:07:08:09:0a:0b"),
                            (2447, "0c:0d:0e:0f:10:11")):
            # Wait for the disconnect to happen.
            success, state, elapsed_seconds = \
                    self.context.client.wait_for_service_states(
                            network_WiFi_ChannelHop.TEST_SSID,
                            ['idle'], 30)

            # Change channels on the AP.  This happens in full view of the DUT
            # and the AP deauths everyone as it exits.
            ap_config = hostap_config.HostapConfig(
                    ssid=network_WiFi_ChannelHop.TEST_SSID,
                    frequency=freq,
                    mode=hostap_config.HostapConfig.MODE_11B,
                    bssid=bssid)
            self.context.configure(ap_config)

            # Wait for the DUT to scan and acquire the AP at the new
            # frequency.
            success, state, elapsed_seconds = \
                    self.context.client.wait_for_service_states(
                            network_WiFi_ChannelHop.TEST_SSID,
                            ['ready', 'portal', 'online'], 30)
            if not success:
                raise error.TestFail(
                        'Failed to connect to "%s" in %f seconds (state=%s)' %
                        (network_WiFi_ChannelHop.TEST_SSID, elapsed_seconds,
                         state))

            # Verify that we're connected.
            self.context.assert_ping_from_dut()

            # Verify that the client switched to new frequency
            self.context.client.check_iw_link_value(
                    iw_runner.IW_LINK_KEY_FREQUENCY,
                    freq)
            self.context.router.deconfig()
