# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import time

from autotest_lib.client.common_lib import error
from autotest_lib.server import autotest
from autotest_lib.server import site_linux_system
from autotest_lib.server.cros.network import hostap_config
from autotest_lib.server.cros.network import wifi_cell_test_base
from autotest_lib.client.common_lib.cros.network \
         import chrome_net_constants

class network_WiFi_RoamSuspendEndToEnd(wifi_cell_test_base.WiFiCellTestBase):
    """Base class that configures two APs with the same SSID that will be used
    by RoamWifiEndToEnd client test to test networking UI.

    The test is run in two phases. First, where we configure the first AP and
    trigger the client test. Second, where we Suspend/Resume the DUT using servo
    and configure a second AP, tear down the first and trigger client side test.

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


    def _do_suspend_deconfig(self, timeout_seconds):
        """Suspend the DUT and deconfigure the AP.

        @param timeout_seconds: Number of seconds to suspend the DUT.

        """
        if self._host.servo.get('lid_open') == 'not_applicable':
            self.context.client.do_suspend_bg(timeout_seconds)
            self.context.router.deconfig_aps(instance=0)
        else:
            self._host.servo.lid_close()
            self._host.wait_down(timeout=timeout_seconds)
            self.context.router.deconfig_aps(instance=0)
            self._host.servo.lid_open()
            self._host.wait_up(timeout=timeout_seconds)


    def run_once(self, host):
        """Set up two APs, run the client side test and then exit.

        """
        self.context.router.require_capabilities(
                [site_linux_system.LinuxSystem.CAPABILITY_MULTI_AP])
        self.context.router.deconfig()
        self._host = host

        if not self._host.servo:
            raise error.TestNAError(
                'Servo object returned None. Check if servo is missing or bad')

        # Configure first AP with channel 5 and mode G and default ssid.
        self._config_ap(5, hostap_config.HostapConfig.MODE_11G)

        client_at = autotest.Autotest(self._host)
        ssid = self.context.router.get_ssid(instance=0)
        time.sleep(chrome_net_constants.LONG_TIMEOUT)
        client_at.run_test('network_RoamWifiEndToEnd',
                           ssid=ssid, test=chrome_net_constants.OPEN_CONNECT)

        # Configure second AP with channel 149, mode N and same ssid as before.
        self._config_ap(149, hostap_config.HostapConfig.MODE_11N_PURE, ssid)
        ssid = self.context.router.get_ssid(instance=1)

        # Suspend the DUT for 15 seconds and tear down the first AP.
        self._do_suspend_deconfig(15)
        client_at.run_test('network_RoamWifiEndToEnd',
                           ssid=ssid, test=chrome_net_constants.OPEN_ROAM)
        self.context.router.deconfig()
