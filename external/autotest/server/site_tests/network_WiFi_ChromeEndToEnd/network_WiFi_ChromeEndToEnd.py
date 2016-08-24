# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.server import autotest
from autotest_lib.server import site_linux_system
from autotest_lib.server.cros.network import hostap_config
from autotest_lib.server.cros.network import wifi_cell_test_base


class network_WiFi_ChromeEndToEnd(wifi_cell_test_base.WiFiCellTestBase):
    """Test that two APs can be configured at the same time and used by the
    ChromeWifiEndToEnd client test to test the UI.

    """
    version = 1


    def _config_ap(self, channel):
        """Configure an AP with given parameters.

        @param ssid: Name to be assigned to the Wifi network.
        @param channel: int Wifi channel to conduct test on.

        """
        n_mode = hostap_config.HostapConfig.MODE_11N_MIXED
        ap_config = hostap_config.HostapConfig(channel=channel, mode=n_mode)
        self.context.configure(ap_config, multi_interface=True)


    def run_once(self, host, test):
        """Set up two APs, run the client side test and then exit.

        @param host: A host object representing the DUT.

        """
        self.context.router.require_capabilities(
                [site_linux_system.LinuxSystem.CAPABILITY_MULTI_AP])
        self.context.router.deconfig()

        self._config_ap(6)
        self._config_ap(149)

        self._host = host
        client_at = autotest.Autotest(self._host)
        client_at.run_test('network_ChromeWifiEndToEnd',
                           ssid_1=self.context.router.get_ssid(instance=0),
                           ssid_2=self.context.router.get_ssid(instance=1),
                           test=test)
        self.context.router.deconfig()
