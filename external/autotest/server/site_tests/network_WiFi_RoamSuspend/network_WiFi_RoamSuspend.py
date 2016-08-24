# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import time

from autotest_lib.client.common_lib.cros.network import xmlrpc_datatypes
from autotest_lib.server.cros.network import hostap_config
from autotest_lib.server.cros.network import wifi_cell_test_base

class network_WiFi_RoamSuspend(wifi_cell_test_base.WiFiCellTestBase):
    """Tests roaming to an AP that changes while we're suspended.

    This test:
    1) Sets up a network with a single BSS.
    2) Connects the DUT to that network and that particular BSS.
    3) Places the DUT in suspend-to-RAM
    4) Replaces the BSS with another BSS on the same SSID.
    5) Watches to make sure the DUT connects to this BSS on resume.

    """

    version = 1

    FREQUENCY_1 = 2412
    FREQUENCY_2 = 5240
    BSSID_1 = "00:01:02:03:04:05"
    BSSID_2 = "06:07:08:09:0a:0b"


    def run_once(self):
        """Test body."""
        logging.info("- Set up AP, connect.")
        self.context.configure(hostap_config.HostapConfig(
                frequency=network_WiFi_RoamSuspend.FREQUENCY_1,
                mode=hostap_config.HostapConfig.MODE_11B,
                bssid=network_WiFi_RoamSuspend.BSSID_1))
        router_ssid = self.context.router.get_ssid()
        self.context.assert_connect_wifi(xmlrpc_datatypes.AssociationParameters(
                ssid=router_ssid))

        # For this short of a duration, the DUT should still consider itself
        # connected to the AP and simply resume without re-associating or
        # reconnect quickly enough without intervention from the connection
        # manager that it appears to remain connected.
        logging.info("- Short suspend, verify we're still connected.")
        self.context.client.do_suspend(10)
        self.context.assert_ping_from_dut()

        logging.info("- Reconfigure the AP during longer suspend.")
        self.context.client.do_suspend_bg(20)
        # Locally, let's wait 15 seconds to make sure the DUT is really asleep
        # before we proceed.
        time.sleep(15)
        self.context.configure(hostap_config.HostapConfig(
                ssid=router_ssid,
                frequency=network_WiFi_RoamSuspend.FREQUENCY_2,
                mode=hostap_config.HostapConfig.MODE_11A,
                bssid=network_WiFi_RoamSuspend.BSSID_2))

        logging.info("- Verify that we roam to same network w/new parameters.")
        self.context.wait_for_connection(router_ssid,
                                         network_WiFi_RoamSuspend.FREQUENCY_2)
        self.context.router.deconfig()
