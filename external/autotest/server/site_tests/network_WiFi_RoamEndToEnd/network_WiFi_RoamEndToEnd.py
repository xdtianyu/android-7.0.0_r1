# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.server import autotest
from autotest_lib.server import site_linux_system
from autotest_lib.server.cros.network import hostap_config
from autotest_lib.server.cros.network import wifi_cell_test_base
from autotest_lib.client.common_lib.cros.network \
         import chrome_net_constants


class network_WiFi_RoamEndToEnd(wifi_cell_test_base.WiFiCellTestBase):
    """Base class that configures two APs with the same SSID that will be used
    by RoamWifiEndToEnd client test to test the UI.

    The test is run in two phases. First, where we configure the first AP and
    trigger the client test. Second, where we configure the other AP, tear
    down the first AP and trigger the client side test again.

    """
    version = 1


    def _config_ap(self, channel, mode, ssid=None):
        """Configure an AP with the given parameters.

        @param channel: int Wifi channel to conduct test on.
        @param ssid: Name to be assigned to the Wifi network.
        @param mode: Mode for the AP configuration.

        """
        ap_config = hostap_config.HostapConfig(channel=channel, mode=mode,
                                               ssid=ssid)
        self.context.configure(ap_config, multi_interface=True)


    def run_once(self):
        """Set up two APs, run the client side test and then exit.

        @param host: A host object representing the DUT.
        @param test: Test to be run on the client side.

        """
        self.context.router.require_capabilities(
                [site_linux_system.LinuxSystem.CAPABILITY_MULTI_AP])
        self.context.router.deconfig()

        # Configure first AP with channel 5 and mode G and default ssid.
        self._config_ap(5, hostap_config.HostapConfig.MODE_11G)
        self._host = self.context.client.host

        client_at = autotest.Autotest(self._host)
        ssid = self.context.router.get_ssid(instance=0)
        client_at.run_test('network_RoamWifiEndToEnd',
                           tag='connect',
                           ssid=ssid,
                           test=chrome_net_constants.OPEN_CONNECT)
        # Configure second AP with channel 149, mode N and same ssid as before.
        self._config_ap(149, hostap_config.HostapConfig.MODE_11N_PURE, ssid)
        ssid = self.context.router.get_ssid(instance=1)

        # Bring down the AP that the device is connected to.
        self.context.router.deconfig_aps(instance=0)
        client_at.run_test('network_RoamWifiEndToEnd',
                           tag='roam',
                           ssid=ssid,
                           test=chrome_net_constants.OPEN_ROAM)

        self.context.router.deconfig()
