# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import time


from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros.network import ping_runner
from autotest_lib.client.common_lib.cros.network import xmlrpc_datatypes
from autotest_lib.server.cros.network import netperf_runner
from autotest_lib.server.cros.network import netperf_session
from autotest_lib.server.cros.network import wifi_cell_test_base
from autotest_lib.server.cros.bluetooth import bluetooth_device


class network_WiFi_BluetoothScanPerf(wifi_cell_test_base.WiFiCellTestBase):
    """Test the effect of bluetooth scanning on wifi performance.

    Conducts a performance test for a set of specified router configurations
    while scanning for bluetooth devices and reports results as keyval pairs.

    """

    version = 1

    NETPERF_CONFIGS = [
            netperf_runner.NetperfConfig(
                       netperf_runner.NetperfConfig.TEST_TYPE_TCP_STREAM),
            netperf_runner.NetperfConfig(
                       netperf_runner.NetperfConfig.TEST_TYPE_TCP_MAERTS),
            netperf_runner.NetperfConfig(
                       netperf_runner.NetperfConfig.TEST_TYPE_UDP_STREAM),
            netperf_runner.NetperfConfig(
                       netperf_runner.NetperfConfig.TEST_TYPE_UDP_MAERTS),
    ]


    def parse_additional_arguments(self, commandline_args, additional_params):
        """Hook into super class to take control files parameters.

        @param commandline_args dict of parsed parameters from the autotest.
        @param additional_params list of HostapConfig objects.

        """
        self._ap_configs = additional_params


    def test_one(self, session, config, ap_config_tag, bt_tag):
        """Run one iteration of wifi testing.

        @param session NetperfSession session
        @param config NetperfConfig config
        @param ap_config_tag string for AP configuration
        @param bt_tag string for BT operation

        """
        get_ping_config = lambda period: ping_runner.PingConfig(
                self.context.get_wifi_addr(), interval=1, count=period)

        logging.info('testing config %s, ap_config %s, BT:%s',
                     config.tag, ap_config_tag, bt_tag)
        test_str = '_'.join([ap_config_tag, bt_tag])
        time.sleep(1)

        signal_level = self.context.client.wifi_signal_level
        signal_description = '_'.join(['signal', test_str])
        self.write_perf_keyval({signal_description: signal_level})

        results = session.run(config)
        if not results:
            logging.error('Failed to take measurement for %s',
                          config.tag)
            return
        values = [result.throughput for result in results]
        self.output_perf_value(config.tag + ' ' + bt_tag, values, units='Mbps',
                               higher_is_better=True,
                               graph=ap_config_tag)
        result = netperf_runner.NetperfResult.from_samples(results)
        self.write_perf_keyval(result.get_keyval(
            prefix='_'.join([config.tag, test_str])))

        # Test latency with ping.
        result_ping = self.context.client.ping(get_ping_config(3))
        self.write_perf_keyval(
            { '_'.join(['ping', test_str]): result_ping.avg_latency })
        logging.info('Ping statistics with %s: %r', bt_tag, result_ping)



    def run_once(self, host):
        """Test body."""
        start_time = time.time()

        # Prepare Bluetooth to scan, but do not start yet.
        bt_device = bluetooth_device.BluetoothDevice(host)
        if not bt_device.reset_on():
            raise error.TestFail('DUT could not be reset to initial state')

        for ap_config in self._ap_configs:
            # Set up the router and associate the client with it.
            self.context.configure(ap_config)
            if ap_config.is_11ac and not self.context.client.is_vht_supported():
                raise error.TestNAError('Client does not have AC support')
            assoc_params = xmlrpc_datatypes.AssociationParameters(
                    ssid=self.context.router.get_ssid(),
                    security_config=ap_config.security_config)
            self.context.assert_connect_wifi(assoc_params)
            session = netperf_session.NetperfSession(self.context.client,
                                                     self.context.router)

            # Warmup the wifi path and measure signal.
            session.warmup_stations()
            ap_config_tag = ap_config.perf_loggable_description

            for config in self.NETPERF_CONFIGS:
                self.test_one(session, config, ap_config_tag, 'BT_quiet')
                if not bt_device.start_discovery():
                    raise error.TestFail('Could not start discovery on DUT')
                try:
                    self.test_one(session, config, ap_config_tag, 'BT_scanning')
                finally:
                    if not bt_device.stop_discovery():
                        logging.warning('Failed to stop discovery on DUT')
                self.test_one(session, config, ap_config_tag, 'BT_quiet_again')

            # Clean up router and client state for the next run.
            self.context.client.shill.disconnect(self.context.router.get_ssid())
            self.context.router.deconfig()

        end_time = time.time()
        logging.info('Running time %0.1f seconds.', end_time - start_time)

