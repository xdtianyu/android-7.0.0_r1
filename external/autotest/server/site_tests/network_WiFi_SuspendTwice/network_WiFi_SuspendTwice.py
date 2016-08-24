# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.common_lib.cros.network import xmlrpc_datatypes
from autotest_lib.server.cros.network import hostap_config
from autotest_lib.server.cros.network import wifi_cell_test_base


class network_WiFi_SuspendTwice(wifi_cell_test_base.WiFiCellTestBase):
    """Test that WiFi works after suspending twice with the device disabled."""

    version = 1


    def run_once(self):
        """Body of the test."""
        self.context.configure(hostap_config.HostapConfig(channel=1))
        assoc_params = xmlrpc_datatypes.AssociationParameters(
                ssid=self.context.router.get_ssid())
        self.context.assert_connect_wifi(assoc_params)
        self.context.client.set_device_enabled(
                self.context.client.wifi_if, False, fail_on_unsupported=True)
        self.context.client.do_suspend(3)
        self.context.client.do_suspend(3)
        self.context.client.set_device_enabled(
                self.context.client.wifi_if, True, fail_on_unsupported=True)
        self.context.wait_for_connection(self.context.router.get_ssid())
