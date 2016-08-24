# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.common_lib.cros.network import ping_runner
from autotest_lib.client.common_lib.cros.network import xmlrpc_datatypes
from autotest_lib.server.cros.network import hostap_config
from autotest_lib.server.cros.network import wifi_cell_test_base


class network_WiFi_WMM(wifi_cell_test_base.WiFiCellTestBase):
    """Test that we can handle different QoS levels."""
    version = 1


    def run_once(self):
        """Body of the test."""
        configuration = hostap_config.HostapConfig(
                frequency=2437,
                mode=hostap_config.HostapConfig.MODE_11G,
                force_wmm=True)
        self.context.configure(configuration)
        assoc_params = xmlrpc_datatypes.AssociationParameters()
        assoc_params.ssid = self.context.router.get_ssid()
        self.context.assert_connect_wifi(assoc_params)
        for qos in ('BE', 'BK', 'VI', 'VO'):
            client_ping_config = ping_runner.PingConfig(
                    self.context.get_wifi_addr(), qos=qos)
            server_ping_config = ping_runner.PingConfig(
                    self.context.client.wifi_ip, qos=qos)
            self.context.assert_ping_from_dut(ping_config=client_ping_config)
            self.context.assert_ping_from_server(ping_config=server_ping_config)
        self.context.client.shill.disconnect(assoc_params.ssid)
        self.context.router.deconfig()
