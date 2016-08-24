# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros.network import ping_runner
from autotest_lib.client.common_lib.cros.network import xmlrpc_datatypes
from autotest_lib.server.cros.network import hostap_config
from autotest_lib.server.cros.network import wifi_cell_test_base


class network_WiFi_OverlappingBSSScan(wifi_cell_test_base.WiFiCellTestBase):
    """Test that background scan backs off when there is foreground traffic."""
    version = 1

    OBSS_SCAN_SAMPLE_PERIOD_SECONDS = 100
    NO_OBSS_SCAN_SAMPLE_PERIOD_SECONDS = 10
    PING_INTERVAL_SECONDS = 0.1
    LATENCY_MARGIN_MS = 150
    THRESHOLD_BASELINE_LATENCY_MS = 100
    WIFI_FREQUENCY = 2437


    @classmethod
    def get_ap_config(cls, scenario_name, use_obss):
        """Returns a HostapConfig object based on the given parameters.

        @param scenario_name: string describing a portion of this test.
        @param use_obss: bool indicating if the AP should ask clients to
            perform OBSS scans.
        @return HostapConfig which incorporates the given parameters.

        """
        return hostap_config.HostapConfig(
            frequency=cls.WIFI_FREQUENCY,
            mode=hostap_config.HostapConfig.MODE_11N_PURE,
            n_capabilities=[
                hostap_config.HostapConfig.N_CAPABILITY_GREENFIELD,
                hostap_config.HostapConfig.N_CAPABILITY_HT40
            ],
            obss_interval=10 if use_obss else None,
            scenario_name=scenario_name)


    def run_once(self):
        """Body of the test."""
        get_assoc_params = lambda: xmlrpc_datatypes.AssociationParameters(
                ssid=self.context.router.get_ssid())
        get_ping_config = lambda period: ping_runner.PingConfig(
                self.context.get_wifi_addr(),
                interval=self.PING_INTERVAL_SECONDS,
                count=int(period / self.PING_INTERVAL_SECONDS))
        # Gather some statistics about ping latencies without scanning going on.
        self.context.configure(self.get_ap_config('obss_disabled', False))
        self.context.assert_connect_wifi(get_assoc_params())
        logging.info('Pinging router without OBSS scans for %d seconds.',
                     self.NO_OBSS_SCAN_SAMPLE_PERIOD_SECONDS)
        result_no_obss_scan = self.context.client.ping(
                get_ping_config(self.NO_OBSS_SCAN_SAMPLE_PERIOD_SECONDS))
        logging.info('Ping statistics without OBSS scans: %r',
                     result_no_obss_scan)
        if result_no_obss_scan.max_latency > self.THRESHOLD_BASELINE_LATENCY_MS:
            raise error.TestFail('RTT latency is too high even without '
                                 'OBSS scans: %f' %
                                 result_no_obss_scan.max_latency)

        self.context.client.shill.disconnect(self.context.router.get_ssid())

        # Re-configure the AP for OBSS and repeat the ping test.
        self.context.configure(self.get_ap_config('obss_enabled', True))
        self.context.router.start_capture(
          self.WIFI_FREQUENCY, filename='obss_enabled.pcap')

        self.context.assert_connect_wifi(get_assoc_params())
        logging.info('Pinging router with OBSS scans for %d seconds.',
                     self.OBSS_SCAN_SAMPLE_PERIOD_SECONDS)
        result_obss_scan = self.context.client.ping(
                get_ping_config(self.OBSS_SCAN_SAMPLE_PERIOD_SECONDS))
        logging.info('Ping statistics with OBSS scans: %r', result_obss_scan)
        self.context.router.stop_capture()

        if not self.context.router.detect_client_coexistence_report(
                self.context.client.wifi_mac):
            raise error.TestFail('No coexistence action frames detected '
                                 'from the client.')

        self.context.client.shill.disconnect(self.context.router.get_ssid())
        self.context.router.deconfig()
        # Dwell time for scanning is usually configured to be around 100 ms,
        # since this is also the standard beacon interval.  Tolerate spikes in
        # latency up to 200 ms as a way of asking that our PHY be servicing
        # foreground traffic regularly during background scans.
        if (result_obss_scan.max_latency >
                self.LATENCY_MARGIN_MS + result_no_obss_scan.avg_latency):
            raise error.TestFail('Significant difference in rtt due to OBSS: '
                                 '%.1f > %.1f + %d' %
                                 (result_obss_scan.max_latency,
                                  result_no_obss_scan.avg_latency,
                                  self.LATENCY_MARGIN_MS))
