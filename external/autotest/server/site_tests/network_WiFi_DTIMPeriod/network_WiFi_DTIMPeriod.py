# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.common_lib.cros.network import iw_runner
from autotest_lib.client.common_lib.cros.network import xmlrpc_datatypes
from autotest_lib.server.cros.network import hostap_config
from autotest_lib.server.cros.network import wifi_cell_test_base


class network_WiFi_DTIMPeriod(wifi_cell_test_base.WiFiCellTestBase):
    """Test that we understand the routers negotiated DTIM period."""
    version = 1


    def run_once(self):
        """DTIM period test.

        DTIM stands for delivery traffic information message and refers to
        the number of beacons between DTIMS.  For instance, a DTIM period
        of 1 would indicate that every beacon should have a DTIM element.
        The default DTIM period value is 2.

        This flag is used in combination with powersave mode as follows:
        1) A client goes into powersave mode and notifies the router.
        2) While in powersave mode, the client turns off as much as possible;
           the AP is supposed to buffer unicast traffic.
        3) The client wakes up to receive beacons, which may include a DTIM
           notification.
        4) On receiving such a notification, the client should
           stay up to recieve the pending frames.

        """
        dtim_val = 5
        configuration = hostap_config.HostapConfig(
                frequency=2437,
                mode=hostap_config.HostapConfig.MODE_11G,
                dtim_period=dtim_val)
        self.context.configure(configuration)
        assoc_params = xmlrpc_datatypes.AssociationParameters()
        assoc_params.ssid = self.context.router.get_ssid()
        self.context.client.powersave_switch(True)
        self.context.assert_connect_wifi(assoc_params)
        self.context.client.check_iw_link_value(
                iw_runner.IW_LINK_KEY_DTIM_PERIOD,
                dtim_val)
        self.context.assert_ping_from_dut()
        self.context.client.shill.disconnect(assoc_params.ssid)
        self.context.client.powersave_switch(False)
        self.context.router.deconfig()
