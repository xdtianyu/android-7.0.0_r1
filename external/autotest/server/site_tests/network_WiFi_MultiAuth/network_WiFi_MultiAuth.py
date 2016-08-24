# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.common_lib.cros.network import xmlrpc_datatypes
from autotest_lib.client.common_lib.cros.network import xmlrpc_security_types
from autotest_lib.server.cros.network import hostap_config
from autotest_lib.server.cros.network import wifi_cell_test_base


class network_WiFi_MultiAuth(wifi_cell_test_base.WiFiCellTestBase):
    """Test our ability to disambiguate similar networks.

    Test that we can distinguish between networks with different
    security and identical SSIDs.

    """
    version = 1

    TEST_SSID = 'an ssid'


    def run_once(self):
        """Test body."""
        wpa_config = xmlrpc_security_types.WPAConfig(
                psk='chromeos',
                wpa_mode=xmlrpc_security_types.WPAConfig.MODE_PURE_WPA,
                wpa_ciphers=[xmlrpc_security_types.WPAConfig.CIPHER_CCMP])
        ap_config0 = hostap_config.HostapConfig(
                ssid=self.TEST_SSID,
                frequency=2412,
                mode=hostap_config.HostapConfig.MODE_11G,
                scenario_name='open_network')
        client_config0 = xmlrpc_datatypes.AssociationParameters(
                ssid=self.TEST_SSID)
        ap_config1 = hostap_config.HostapConfig(
                ssid=self.TEST_SSID,
                frequency=2412,
                mode=hostap_config.HostapConfig.MODE_11G,
                security_config=wpa_config,
                scenario_name='wpa_network')
        client_config1 = xmlrpc_datatypes.AssociationParameters(
                ssid=self.TEST_SSID,
                security_config=wpa_config)
        self.context.configure(ap_config0)
        self.context.configure(ap_config1, multi_interface=True)
        self.context.assert_connect_wifi(client_config0)
        self.context.assert_ping_from_dut(ap_num=0)
        self.context.assert_connect_wifi(client_config1)
        self.context.assert_ping_from_dut(ap_num=1)
