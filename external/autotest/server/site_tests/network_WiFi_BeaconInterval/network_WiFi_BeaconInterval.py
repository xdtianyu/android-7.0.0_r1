# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.common_lib.cros.network import iw_runner
from autotest_lib.client.common_lib.cros.network import xmlrpc_datatypes
from autotest_lib.server.cros.network import hostap_config
from autotest_lib.server.cros.network import wifi_cell_test_base


class network_WiFi_BeaconInterval(wifi_cell_test_base.WiFiCellTestBase):
    """Test that we understand the routers negotiated beacon interval."""
    version = 1


    def run_once(self):
        """Body of the test."""
        bint_val = 200
        configuration = hostap_config.HostapConfig(
                channel=6,
                mode=hostap_config.HostapConfig.MODE_11B,
                beacon_interval=bint_val)
        self.context.configure(configuration)
        assoc_params = xmlrpc_datatypes.AssociationParameters()
        assoc_params.ssid = self.context.router.get_ssid()
        self.context.assert_connect_wifi(assoc_params)
        self.context.client.check_iw_link_value(
                iw_runner.IW_LINK_KEY_BEACON_INTERVAL,
                bint_val)
        self.context.assert_ping_from_dut()
        self.context.client.shill.disconnect(assoc_params.ssid)
        self.context.router.deconfig()
