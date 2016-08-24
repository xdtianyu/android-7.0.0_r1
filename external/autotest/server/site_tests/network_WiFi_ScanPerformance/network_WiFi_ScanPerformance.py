# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.common_lib.cros.network import xmlrpc_datatypes
from autotest_lib.server.cros.network import hostap_config
from autotest_lib.server.cros.network import wifi_cell_test_base


class network_WiFi_ScanPerformance(wifi_cell_test_base.WiFiCellTestBase):
    """Performance test for scanning operation in various setup"""
    version = 1

    def run_once(self):
        """Sets up a router, scan for APs """

        # Default router configuration
        router_conf = hostap_config.HostapConfig(channel=6);
        freq = hostap_config.HostapConfig.get_frequency_for_channel(6)
        self.context.configure(router_conf)
        ssids = [self.context.router.get_ssid()]

        # Single channel scan
        scan_time = self.context.client.timed_scan(frequencies=[freq],
                ssids=ssids, scan_timeout_seconds=10)
        self.write_perf_keyval({'scan_time_single_channel_scan': scan_time})

        # Foreground full scan
        scan_time = self.context.client.timed_scan(frequencies=[], ssids=ssids,
                                                   scan_timeout_seconds=10)
        self.write_perf_keyval({'scan_time_foreground_full_scan': scan_time})

        # Background full scan
        client_conf = xmlrpc_datatypes.AssociationParameters(
                ssid=self.context.router.get_ssid())
        self.context.assert_connect_wifi(client_conf)
        scan_time = self.context.client.timed_scan(frequencies=[], ssids=ssids,
                                                   scan_timeout_seconds=15)
        self.write_perf_keyval({'scan_time_background_full_scan': scan_time})

        # Deconfigure router
        self.context.router.deconfig()
