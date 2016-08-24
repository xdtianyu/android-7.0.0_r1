# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.common_lib.cros.network import ping_runner
from autotest_lib.client.common_lib.cros.network import xmlrpc_datatypes
from autotest_lib.server.cros.network import hostap_config
from autotest_lib.server.cros.network import wifi_cell_test_base


class network_WiFi_RxFrag(wifi_cell_test_base.WiFiCellTestBase):
    """Test that the DUT can reassemble packet fragments."""
    version = 1


    def run_once(self):
        """Test body.

        When fragthreshold is set, packets larger than the threshold are
        broken up by the AP and sent in fragments. The DUT needs to reassemble
        these fragments to reconstruct the original packets before processing
        them.

        """
        configuration = hostap_config.HostapConfig(
                frequency=2437,
                mode=hostap_config.HostapConfig.MODE_11G,
                frag_threshold=256)
        self.context.configure(configuration)
        self.context.router.start_capture(configuration.frequency)
        assoc_params = xmlrpc_datatypes.AssociationParameters()
        assoc_params.ssid = self.context.router.get_ssid()
        self.context.assert_connect_wifi(assoc_params)
        build_config = lambda size: ping_runner.PingConfig(
                self.context.client.wifi_ip, size=size)
        self.context.assert_ping_from_server(ping_config=build_config(256))
        self.context.assert_ping_from_server(ping_config=build_config(512))
        self.context.assert_ping_from_server(ping_config=build_config(1024))
        self.context.assert_ping_from_server(ping_config=build_config(1500))
        self.context.client.shill.disconnect(assoc_params.ssid)
        self.context.router.deconfig()
