# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.common_lib.cros.network import xmlrpc_datatypes
from autotest_lib.client.common_lib.cros.network  import xmlrpc_security_types
from autotest_lib.server.cros.network import hostap_config
from autotest_lib.server.cros.network import wifi_cell_test_base


class network_WiFi_SecChange(wifi_cell_test_base.WiFiCellTestBase):
    """Test that we can connect to a BSS despite security changes."""

    version = 1

    TEST_SSID = 'My_security_changes'


    def run_once(self):
        """Test body."""
        wpa_config = xmlrpc_security_types.WPAConfig(
                psk='chromeos',
                wpa_mode=xmlrpc_security_types.WPAConfig.MODE_MIXED_WPA,
                wpa_ciphers=[xmlrpc_security_types.WPAConfig.CIPHER_TKIP,
                             xmlrpc_security_types.WPAConfig.CIPHER_CCMP],
                wpa2_ciphers=[xmlrpc_security_types.WPAConfig.CIPHER_CCMP])
        ap_config = hostap_config.HostapConfig(
                    ssid=self.TEST_SSID,
                    frequency=2412,
                    mode=hostap_config.HostapConfig.MODE_11B,
                    security_config=wpa_config)
        self.context.configure(ap_config)
        assoc_params = xmlrpc_datatypes.AssociationParameters(
                ssid=self.context.router.get_ssid(),
                security_config=wpa_config)
        self.context.assert_connect_wifi(assoc_params)
        self.context.assert_ping_from_dut()
        self.context.client.shill.disconnect(assoc_params.ssid)
        # This deconfig erases the state stored in the router around WPA.
        self.context.router.deconfig()
        # Now we change the same SSID to be an open network.
        ap_config = hostap_config.HostapConfig(
                    ssid=self.TEST_SSID,
                    frequency=2412,
                    mode=hostap_config.HostapConfig.MODE_11B)
        self.context.configure(ap_config)
        assoc_params = xmlrpc_datatypes.AssociationParameters(
                ssid=self.context.router.get_ssid())
        self.context.assert_connect_wifi(assoc_params)
        self.context.assert_ping_from_dut()
        self.context.client.shill.disconnect(assoc_params.ssid)
        self.context.router.deconfig()
