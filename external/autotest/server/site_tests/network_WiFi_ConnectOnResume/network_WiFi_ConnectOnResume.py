# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import time

from autotest_lib.client.common_lib.cros.network import xmlrpc_datatypes
from autotest_lib.server.cros.network import hostap_config
from autotest_lib.server.cros.network import wifi_cell_test_base

class network_WiFi_ConnectOnResume(wifi_cell_test_base.WiFiCellTestBase):
    """
    Tests our behavior after a resume when not connected before the suspend.

    This test:
        1) Sets up a network with a single hidden BSS.
        2) Connects the DUT to that network and that particular BSS.
        3) Takes down the BSS in view of the DUT.
        4) Waits for scan cache results from the device to expire.
        5) Places the DUT in suspend-to-RAM
        6) Bring the same BSS back up.
        7) Resumes the DUT.
        8) Watches to make sure the DUT connects to this BSS on resume.

    Note that since the BSS is hidden, and wpa_supplicant does not
    know to explicitly scan for hidden BSS's, this means that shill
    must be triggering the scans.

    """

    version = 1


    def run_once(self):
        """Test body."""
        self.context.configure(
                hostap_config.HostapConfig(channel=1, hide_ssid=True))
        router_ssid = self.context.router.get_ssid()
        assoc_params = xmlrpc_datatypes.AssociationParameters(
                ssid=router_ssid, is_hidden=True)
        self.context.assert_connect_wifi(assoc_params)
        self.context.router.deconfig()
        self.context.client.wait_for_ssid_vanish(router_ssid)
        self.context.client.do_suspend_bg(20)
        # Locally, let's wait 15 seconds to make sure the DUT is really asleep
        # before we proceed.
        time.sleep(15)
        self.context.configure(hostap_config.HostapConfig(
            channel=1, ssid=router_ssid, hide_ssid=True))
        # When we resume, we should see the device automatically connect,
        # despite the absence of beacons.
        self.context.wait_for_connection(router_ssid)
