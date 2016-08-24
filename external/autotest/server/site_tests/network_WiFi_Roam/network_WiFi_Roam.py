# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.server.cros.network import wifi_cell_test_base

class network_WiFi_Roam(wifi_cell_test_base.WiFiCellTestBase):
    """Tests roaming to an AP that changes while the client is awake

    This test run seeks to associate the DUT with an AP with a set of
    association parameters, create a second AP with a second set of
    parameters but the same SSID, and shut down the first DUT.  We
    seek to observe that the DUT successfully connects to the second
    AP in a reasonable amount of time.
    """

    version = 1

    def parse_additional_arguments(self, commandline_args, additional_params):
        """Hook into super class to take control files parameters.

        @param commandline_args dict of parsed parameters from the autotest.
        @param additional_params tuple of (HostapConfig,
                                           AssociationParameters).

        """
        self._router0_conf, self._router1_conf, self._client_conf = (
                additional_params)


    def run_once(self):
        """Test body."""
        # Configure the inital AP.
        self.context.configure(self._router0_conf)
        router_ssid = self.context.router.get_ssid()

        # Connect to the inital AP.
        self._client_conf.ssid = router_ssid
        self.context.assert_connect_wifi(self._client_conf)

        # Setup a second AP with the same SSID.
        self._router1_conf.ssid = router_ssid
        self.context.configure(self._router1_conf, multi_interface=True)

        # Tear down the AP instance that the DUT is currently connected to.
        self.context.router.deconfig_aps(instance=0)

        # Expect that the DUT will re-connect to the new AP.
        self.context.wait_for_connection(router_ssid,
                                         self._router1_conf.frequency)
        self.context.router.deconfig()
