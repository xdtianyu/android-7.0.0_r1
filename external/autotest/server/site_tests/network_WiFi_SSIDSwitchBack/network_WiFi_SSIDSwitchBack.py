# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros.network import iw_runner
from autotest_lib.client.common_lib.cros.network import xmlrpc_datatypes
from autotest_lib.server.cros.network import hostap_config
from autotest_lib.server.cros.network import wifi_cell_test_base

class network_WiFi_SSIDSwitchBack(wifi_cell_test_base.WiFiCellTestBase):
    """Tests roaming to a previous AP when current AP disappears."""

    version = 1

    FREQUENCY_1 = 2412
    FREQUENCY_2 = 2437
    BSSID_1 = "00:01:02:03:04:05"
    BSSID_2 = "06:07:08:09:0a:0b"
    SSID_1 = "InsideADogItsTooDarkToRead"
    SSID_2 = "HeReallyIsAnIdiot"
    CONNECTED_STATE = 'ready', 'portal', 'online'


    def configure_connect_verify_deconfig_wait(self, ssid, freq, mode, bssid):
        """Configure an AP, connect to it, then tear it all down, again.

        This method does the following: configures the AP, connects to it and
        verifies the connection, deconfigures the AP and waits for the
        disconnect to complete.

        @param ssid string SSID for the new connection.
        @param freq int Frequency which the AP is to support.
        @param mode string AP mode from hostap_config.HostapConfig.MODE_*.
        @param bssid string BSSID for the new connection.

        """
        # Change channels on the AP.  This happens in full view of the DUT
        # and the AP deauths everyone as it exits.
        ap_config = hostap_config.HostapConfig(ssid=ssid, frequency=freq,
                                               mode=mode, bssid=bssid)
        self.context.configure(ap_config)
        assoc_params = xmlrpc_datatypes.AssociationParameters(
                ssid=self.context.router.get_ssid())
        self.context.assert_connect_wifi(assoc_params)

        self.context.assert_ping_from_dut()  # Verify that we're connected.
        self.context.client.check_iw_link_value(
                iw_runner.IW_LINK_KEY_FREQUENCY,
                freq)  # Verify that the client switched to new frequency

        # Deconfig and wait for the DUT to disconnect and end up at 'idle'.
        self.context.router.deconfig()
        success, state, elapsed_seconds = \
                self.context.client.wait_for_service_states(
                        network_WiFi_SSIDSwitchBack.SSID_1, ['idle'], 30)


    def run_once(self):
        """Test body."""
        # Connect to the first AP.  This just guarantees that this AP has
        # been placed in the connection manager profile.  Then deconfig.
        self.configure_connect_verify_deconfig_wait(
                network_WiFi_SSIDSwitchBack.SSID_1,
                network_WiFi_SSIDSwitchBack.FREQUENCY_1,
                hostap_config.HostapConfig.MODE_11B,
                network_WiFi_SSIDSwitchBack.BSSID_1)

        # Configure and connect to the second AP.  Then deconfig.
        self.configure_connect_verify_deconfig_wait(
                network_WiFi_SSIDSwitchBack.SSID_2,
                network_WiFi_SSIDSwitchBack.FREQUENCY_2,
                hostap_config.HostapConfig.MODE_11G,
                network_WiFi_SSIDSwitchBack.BSSID_2)

        # Bring the first AP back up.
        ap_config = hostap_config.HostapConfig(
                ssid=network_WiFi_SSIDSwitchBack.SSID_1,
                frequency=network_WiFi_SSIDSwitchBack.FREQUENCY_1,
                mode=hostap_config.HostapConfig.MODE_11B,
                bssid=network_WiFi_SSIDSwitchBack.BSSID_1)
        self.context.configure(ap_config)

        # Instead of explicitly connecting, just wait to see if the DUT
        # re-connects by itself
        success, state, elapsed_seconds = \
                self.context.client.wait_for_service_states(
                        network_WiFi_SSIDSwitchBack.SSID_1,
                        network_WiFi_SSIDSwitchBack.CONNECTED_STATE, 30)
        if (not success or
            state not in network_WiFi_SSIDSwitchBack.CONNECTED_STATE):
            raise error.TestFail(
                    'Failed to connect to "%s" in %f seconds (state=%s)' %
                    (network_WiFi_SSIDSwitchBack.SSID_1, elapsed_seconds,
                     state))

        # Verify that we're connected.
        self.context.assert_ping_from_dut()

        # Verify that the client switched to the original frequency
        self.context.client.check_iw_link_value(
                iw_runner.IW_LINK_KEY_FREQUENCY,
                network_WiFi_SSIDSwitchBack.FREQUENCY_1)
        self.context.router.deconfig()
