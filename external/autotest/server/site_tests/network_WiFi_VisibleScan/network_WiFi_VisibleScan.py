# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros.network import tcpdump_analyzer
from autotest_lib.client.common_lib.cros.network import xmlrpc_datatypes
from autotest_lib.server.cros.network import hostap_config
from autotest_lib.server.cros.network import packet_capturer
from autotest_lib.server.cros.network import wifi_cell_test_base


class network_WiFi_VisibleScan(wifi_cell_test_base.WiFiCellTestBase):
    """Test scanning behavior when no hidden SSIDs are configured."""

    version = 1

    BROADCAST_SSID = ''

    def parse_additional_arguments(self, commandline_args, additional_params):
        """
        Hook into super class to take control files parameters.

        @param commandline_args: dict of parsed parameters from the autotest.
        @param additional_params: list of HostapConfig objects.

        """
        self._ap_configs = additional_params


    def run_once(self):
        """Test body."""
        ap_config = hostap_config.HostapConfig(channel=1)
        # Set up the router and associate the client with it.
        self.context.configure(ap_config)
        self.context.router.start_capture(
                ap_config.frequency,
                ht_type=ap_config.ht_packet_capture_mode,
                snaplen=packet_capturer.SNAPLEN_WIFI_PROBE_REQUEST)
        assoc_params = xmlrpc_datatypes.AssociationParameters(
                ssid=self.context.router.get_ssid())
        self.context.assert_connect_wifi(assoc_params)
        results = self.context.router.stop_capture()
        if len(results) != 1:
            raise error.TestError('Expected to generate one packet '
                                  'capture but got %d instead.' %
                                  len(results))
        probe_ssids = tcpdump_analyzer.get_probe_ssids(
                results[0].local_pcap_path,
                probe_sender=self.context.client.wifi_mac)
        expected_ssids = frozenset([self.BROADCAST_SSID])
        permitted_ssids = (expected_ssids |
                frozenset([self.context.router.get_ssid()]))
        # Verify expected ssids are contained in the probe result
        if expected_ssids - probe_ssids:
            raise error.TestError('Expected SSIDs %s, but got %s' %
                                  (expected_ssids, probe_ssids))
        # Verify probe result does not contain any unpermitted ssids
        if probe_ssids - permitted_ssids:
            raise error.TestError('Permitted SSIDs %s, but got %s' %
                                  (permitted_ssids, probe_ssids))
