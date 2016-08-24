# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros.network import ping_runner
from autotest_lib.client.common_lib.cros.network import xmlrpc_datatypes
from autotest_lib.server import site_linux_system
from autotest_lib.server.cros.network import wifi_cell_test_base


class network_WiFi_Regulatory(wifi_cell_test_base.WiFiCellTestBase):
    """Test that the client vacates the channel after notification
    from the AP that it should switch channels."""
    version = 1


    def parse_additional_arguments(self, commandline_args, additional_params):
        """Hook into super class to take control files parameters.

        @param commandline_args dict of parsed parameters from the autotest.
        @param additional_params list of dicts describing router configs.

        """
        self._configurations = additional_params


    def run_once(self):
        """Sets up a router, connects to it, then tests a channel switch."""
        for router_conf, alternate_channel in self._configurations:
            self.context.router.require_capabilities(
                  [site_linux_system.LinuxSystem.
                          CAPABILITY_SEND_MANAGEMENT_FRAME])
            self.context.configure(router_conf)
            self.context.router.start_capture(
                router_conf.frequency,
                filename='chan%d.pcap' % router_conf.channel)
            assoc_params = xmlrpc_datatypes.AssociationParameters()
            assoc_params.ssid = self.context.router.get_ssid()
            self.context.assert_connect_wifi(assoc_params)
            ping_config = ping_runner.PingConfig(
                    self.context.get_wifi_addr(ap_num=0))
            client_mac = self.context.client.wifi_mac
            for attempt in range(10):
                # Since the client might be in power-save, we are not
                # guaranteed it will hear this message the first time around.
                self.context.router.send_management_frame_on_ap(
                        'channel_switch', alternate_channel)

                # Test to see if the router received a deauth message from
                # the client.
                if self.context.router.detect_client_deauth(client_mac):
                    break

                # Otherwise detect the client leaving indirectly by measuring
                # client pings.  This should fail at some point.
                ping_config = ping_runner.PingConfig(
                        self.context.get_wifi_addr(ap_num=0),
                        count=3, ignore_status=True,
                        ignore_result=True)
                result = self.context.client.ping(ping_config)
                if result.loss > 60:
                    break
            else:
                raise error.TestFail('Client never lost connectivity')
            self.context.client.shill.disconnect(assoc_params.ssid)
            self.context.router.deconfig()
