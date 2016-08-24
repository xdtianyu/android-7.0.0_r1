# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import time

from autotest_lib.client.common_lib.cros.network import xmlrpc_datatypes
from autotest_lib.server import site_attenuator
from autotest_lib.server.cros.network import hostap_config
from autotest_lib.server.cros.network import rvr_test_base

class network_WiFi_SSIDMultiSwitchBack(rvr_test_base.RvRTestBase):
    """Tests roaming to an AP when the old one's signal is too weak.

    This test uses a dual-radio Stumpy as the AP and configures the radios to
    broadcast two BSS's with different frequencies on the same SSID.  The DUT
    connects to the first radio, the test attenuates that radio, and the DUT
    is supposed to roam to the second radio.

    This test requires a particular configuration of test equipment:

                                   +--------- StumpyCell/AP ----------+
                                   | chromeX.grover.hostY.router.cros |
                                   |                                  |
                                   |       [Radio 0]  [Radio 1]       |
                                   +--------A-----B----C-----D--------+
        +------ BeagleBone ------+          |     |    |     |
        | chromeX.grover.hostY.  |          |     X    |     X
        | attenuator.cros      [Port0]-[attenuator]    |
        |                      [Port1]----- | ----[attenuator]
        |                      [Port2]-X    |          |
        |                      [Port3]-X    +-----+    |
        |                        |                |    |
        +------------------------+                |    |
                                   +--------------E----F--------------+
                                   |             [Radio 0]            |
                                   |                                  |
                                   |    chromeX.grover.hostY.cros     |
                                   +-------------- DUT ---------------+

    Where antennas A, C, and E are the primary antennas for AP/radio0,
    AP/radio1, and DUT/radio0, respectively; and antennas B, D, and F are the
    auxilliary antennas for AP/radio0, AP/radio1, and DUT/radio0,
    respectively.  The BeagleBone controls 2 attenuators that are connected
    to the primary antennas of AP/radio0 and 1 which are fed into the primary
    and auxilliary antenna ports of DUT/radio 0.  Ports 2 and 3 of the
    BeagleBone as well as the auxillary antennae of AP/radio0 and 1 are
    terminated.

    This arrangement ensures that the attenuator port numbers are assigned to
    the primary radio, first, and the secondary radio, second.  If this happens,
    the ports will be numbered in the order in which the AP's channels are
    configured (port 0 is first, port 1 is second, etc.).

    This test is a de facto test that the ports are configured in that
    arrangement since swapping Port0 and Port1 would cause us to attenuate the
    secondary radio, providing no impetus for the DUT to switch radios and
    causing the test to fail to connect at radio 1's frequency.

    """

    version = 1

    FREQUENCY_0 = 2412
    FREQUENCY_1 = 2462
    PORT_0 = 0  # Port created first (FREQUENCY_0)
    PORT_1 = 1  # Port created second (FREQUENCY_1)


    def run_once(self):
        """Test body."""

        logging.info("- Configure first AP & connect")
        self.context.configure(hostap_config.HostapConfig(
                frequency=network_WiFi_SSIDMultiSwitchBack.FREQUENCY_0,
                mode=hostap_config.HostapConfig.MODE_11G))
        router_ssid = self.context.router.get_ssid()
        self.context.assert_connect_wifi(xmlrpc_datatypes.AssociationParameters(
                ssid=router_ssid))
        self.context.assert_ping_from_dut()

        logging.info('- Configure second AP')
        self.context.configure(hostap_config.HostapConfig(
                ssid=router_ssid,
                frequency=network_WiFi_SSIDMultiSwitchBack.FREQUENCY_1,
                mode=hostap_config.HostapConfig.MODE_11G),
                               multi_interface=True)

        logging.info('- Drop the power on the first AP')
        self.context.attenuator.set_variable_attenuation_on_port(
            network_WiFi_SSIDMultiSwitchBack.PORT_0,
            site_attenuator.Attenuator.MAX_VARIABLE_ATTENUATION)
        time.sleep(50)  # Give DUT time to scan and roam.

        logging.info("- Wait for a connection on the second AP")
        # Instead of explicitly connecting, just wait to see if the DUT
        # connects to the second AP by itself
        self.context.wait_for_connection(
                ssid=router_ssid,
                freq=network_WiFi_SSIDMultiSwitchBack.FREQUENCY_1,
                ap_num=1)

        # Clean up.
        self.context.router.deconfig()
