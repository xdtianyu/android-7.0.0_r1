# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.server import site_linux_system
from autotest_lib.server.cros.network import hostap_config
from autotest_lib.server.cros.network import wifi_cell_test_base


class network_WiFi_MaskedBSSID(wifi_cell_test_base.WiFiCellTestBase):
    """Test behavior around masked BSSIDs."""
    version = 1


    def run_once(self):
        """Test body.

        Set up two APs on the same channel/bssid but with different SSIDs.
        Check that we can see both APs in scan results.

        """
        self.context.router.require_capabilities(
                [site_linux_system.LinuxSystem.CAPABILITY_MULTI_AP_SAME_BAND])
        frequency = 2412
        configurations = [hostap_config.HostapConfig(
                          frequency=frequency,
                          mode=hostap_config.HostapConfig.MODE_11B,
                          bssid='00:11:22:33:44:55',
                          ssid=('CrOS_Masked%d' % i),
                          min_streams=1) for i in range(2)]
        # Create an AP, manually specifying both the SSID and BSSID.
        self.context.configure(configurations[0])
        # Create a second AP that responds to probe requests with the same BSSID
        # but an different SSID.  These APs together are meant to emulate
        # situations that occur with some types of APs which broadcast or
        # respond with more than one (non-empty) SSID.
        self.context.configure(configurations[1], multi_interface=True)
        # We cannot connect to this AP, since there are two separate APs that
        # respond to the same BSSID, but we can test to make sure both SSIDs
        # appears in the scan.
        self.context.client.scan([frequency],
                                 [config.ssid for config in configurations])
        self.context.router.deconfig()
