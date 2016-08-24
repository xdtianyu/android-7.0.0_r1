# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import time

from autotest_lib.client.common_lib.cros.network import xmlrpc_datatypes
from autotest_lib.server.cros.network import wifi_cell_test_base

class network_WiFi_RoamSuspendSSID(wifi_cell_test_base.WiFiCellTestBase):
    """Tests roaming to a new SSID when a previous SSID disappears in suspend.

    This test:
    1) Connects the DUT to a network A
    2) Connects the DUT to a network B while keeping network A around.
    3) Suspend the DUT (while connected to network B).
    4) Deconfigure (take down) network B.
    5) Assert that the DUT automatically connects to network A on resume.

    """

    version = 1

    SUSPEND_TIME_SECONDS = 15


    def parse_additional_arguments(self, commandline_args, additional_params):
        """Hook into super class to take control files parameters.

        @param commandline_args: dict of parsed parameters from the autotest.
        @param additional_params: tuple(HostapConfig, HostapConfig) used as
                networks A and B from the test description.

        """
        self._ap_config0 = additional_params[0]
        self._ap_config1 = additional_params[1]


    def run_once(self):
        """Test body."""
        get_client_config = lambda ssid, ap_config: \
                xmlrpc_datatypes.AssociationParameters(
                        ssid=ssid,
                        security_config=ap_config.security_config)
        self.context.configure(self._ap_config0)
        self.context.configure(self._ap_config1, multi_interface=True)
        self.context.assert_connect_wifi(
                get_client_config(self.context.router.get_ssid(instance=0),
                                  self._ap_config0))
        self.context.assert_connect_wifi(
                get_client_config(self.context.router.get_ssid(instance=1),
                                  self._ap_config1))
        self.context.client.do_suspend_bg(self.SUSPEND_TIME_SECONDS + 5)
        logging.info('Waiting %d seconds for DUT to be fully suspended.',
                     self.SUSPEND_TIME_SECONDS)
        time.sleep(self.SUSPEND_TIME_SECONDS)
        logging.info('Tearing down the most recently connected AP.')
        self.context.router.deconfig_aps(instance=1)
        logging.info('Expect that we connect to our previous connected AP '
                     'on resume.')
        self.context.wait_for_connection(
                self.context.router.get_ssid(instance=0),
                self._ap_config0.frequency)
