# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import time

from autotest_lib.client.common_lib.cros.network import xmlrpc_datatypes
from autotest_lib.server.cros.network import wifi_cell_test_base

class network_WiFi_RoamSuspendTimeout(wifi_cell_test_base.WiFiCellTestBase):
    """Tests behavior on resume during which the client has been de-authed.

    This test places the DUT in a suspend-to-RAM state, de-authenticates the
    sleeping DUT, then makes sure that the DUT detects and corrects this
    de-authentication on resume.

    """
    version = 1


    def parse_additional_arguments(self, commandline_args, additional_params):
        """Hook into super class to take control files parameters.

        @param commandline_args dict of parsed parameters from the autotest.
        @param additional_params HostapConfig

        """
        self._router_conf = additional_params


    def run_once(self):
        """Test body."""
        logging.info("- Set up AP, connect.")
        self.context.configure(self._router_conf)

        client_conf = xmlrpc_datatypes.AssociationParameters(
                ssid=self.context.router.get_ssid(),
                security_config=self._router_conf.security_config)
        self.context.assert_connect_wifi(client_conf)
        self.context.assert_ping_from_dut()

        # Suspend the DUT then, locally, wait 15 seconds to make sure the
        # DUT is really asleep before we proceed.  Then, deauth the DUT
        # while it sleeps.
        logging.info("- Suspend & deauthenticate during suspend.")
        self.context.client.do_suspend_bg(20)
        time.sleep(15)
        self.context.router.deauth_client(self.context.client.wifi_mac)

        # If the DUT realizes that it has been deauthed, then it should
        # reassociate quickly and the ping below should succeed.
        logging.info("- Verify that we roam back to same network.")
        self.context.wait_for_connection(client_conf.ssid)

        self.context.router.deconfig()
