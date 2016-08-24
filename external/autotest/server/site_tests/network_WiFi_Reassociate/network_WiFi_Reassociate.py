# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.common_lib.cros.network import xmlrpc_datatypes
from autotest_lib.server.cros.network import hostap_config
from autotest_lib.server.cros.network import wifi_cell_test_base


class network_WiFi_Reassociate(wifi_cell_test_base.WiFiCellTestBase):
    """Timing test for wpa_supplicant reassociate operation."""
    version = 1

    def run_once(self):
        """Setup and connect to an AP, then perform reassociate test."""
        ap_config = hostap_config.HostapConfig(channel=6)
        self.context.configure(ap_config)
        client_conf = xmlrpc_datatypes.AssociationParameters(
                ssid=self.context.router.get_ssid())
        self.context.assert_connect_wifi(client_conf)
        self.context.client.reassociate(timeout_seconds=10)
