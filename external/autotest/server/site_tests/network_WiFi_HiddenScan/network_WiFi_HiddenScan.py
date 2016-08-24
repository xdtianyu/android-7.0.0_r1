# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros.network import tcpdump_analyzer
from autotest_lib.client.common_lib.cros.network import xmlrpc_datatypes
from autotest_lib.server.cros.network import hostap_config
from autotest_lib.server.cros.network import packet_capturer
from autotest_lib.server.cros.network import wifi_cell_test_base


class network_WiFi_HiddenScan(wifi_cell_test_base.WiFiCellTestBase):
    """Test scanning behavior when a hidden SSID is configured."""

    version = 1

    BROADCAST_SSID = ''

    def run_once(self):
        """Test body."""
        ap_config = hostap_config.HostapConfig(channel=1, hide_ssid=True)
        # Set up the router and associate the client with it.
        self.context.configure(ap_config)
        self.context.router.start_capture(
                ap_config.frequency,
                ht_type=ap_config.ht_packet_capture_mode,
                snaplen=packet_capturer.SNAPLEN_WIFI_PROBE_REQUEST)
        test_ssid=self.context.router.get_ssid()
        assoc_params = xmlrpc_datatypes.AssociationParameters(
                ssid=test_ssid, is_hidden=True)
        self.context.assert_connect_wifi(assoc_params)
        results = self.context.router.stop_capture()
        if len(results) != 1:
            raise error.TestError('Expected to generate one packet '
                                  'capture but got %d instead.' %
                                  len(results))
        probe_ssids = tcpdump_analyzer.get_probe_ssids(
                results[0].local_pcap_path,
                probe_sender=self.context.client.wifi_mac)
        if len(probe_ssids) != 2:
            raise error.TestError('Expected exactly two SSIDs, but got %s' %
                                  probe_ssids)
        if probe_ssids - {self.BROADCAST_SSID, test_ssid}:
            raise error.TestError('Unexpected probe SSIDs: %s' % probe_ssids)
