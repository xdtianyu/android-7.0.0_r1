# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.client.common_lib.cros.network import xmlrpc_datatypes
from autotest_lib.server.cros.network import hostap_config
from autotest_lib.server.cros.network import wifi_cell_test_base


class network_WiFi_Powersave(wifi_cell_test_base.WiFiCellTestBase):
    """Test that we can enter and exit powersave mode without issue."""
    version = 1

    def check_powersave(self, should_be_on):
        """Raises an exception if powersave value is other than expected.

        @param should_be_on: bool True iff powersave mode should be on
                for the WiFi Phy on the device under test.

        """
        expected_state = 'enabled' if should_be_on else 'disabled'
        if should_be_on != self.context.client.powersave_on:
            raise error.TestFail('Expected powersave mode to be %s but it '
                                 'was not.' % expected_state)

        logging.debug('Power save is indeed %s.', expected_state)


    def run_once(self):
        """Test body.

        Powersave mode takes advantage of DTIM intervals, and so the two
        are intimately tied.  See network_WiFi_DTIMPeriod for a discussion
        of their interaction.

        """
        dtim_val = 5
        configuration = hostap_config.HostapConfig(
                frequency=2437,
                mode=hostap_config.HostapConfig.MODE_11G,
                dtim_period=dtim_val)
        self.context.configure(configuration)
        self.check_powersave(False)
        assoc_params = xmlrpc_datatypes.AssociationParameters()
        assoc_params.ssid = self.context.router.get_ssid()
        self.context.client.powersave_switch(True)
        self.check_powersave(True)
        self.context.assert_connect_wifi(assoc_params)
        self.context.assert_ping_from_dut()
        self.context.assert_ping_from_server()
        self.context.client.shill.disconnect(assoc_params.ssid)
        self.context.client.powersave_switch(False)
        self.check_powersave(False)
        self.context.router.deconfig()
