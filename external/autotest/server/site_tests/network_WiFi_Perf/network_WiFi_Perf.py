# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import time

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros.network import xmlrpc_datatypes
from autotest_lib.server.cros.network import netperf_runner
from autotest_lib.server.cros.network import netperf_session
from autotest_lib.server.cros.network import wifi_cell_test_base


class network_WiFi_Perf(wifi_cell_test_base.WiFiCellTestBase):
    """Test maximal achievable bandwidth on several channels per band.

    Conducts a performance test for a set of specified router configurations
    and reports results as keyval pairs.

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


    def run_once(self):
        """Test body."""
        start_time = time.time()
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
            # Conduct the performance tests while toggling powersave mode.
            for power_save in (True, False):
                self.context.client.powersave_switch(power_save)
                session.warmup_stations()
                ps_tag = 'PS%s' % ('on' if power_save else 'off')
                ap_config_tag = '_'.join([ap_config.perf_loggable_description,
                                          ps_tag])
                signal_level = self.context.client.wifi_signal_level
                signal_description = '_'.join([ap_config_tag, 'signal'])
                self.write_perf_keyval({signal_description: signal_level})
                for config in self.NETPERF_CONFIGS:
                    results = session.run(config)
                    if not results:
                        logging.error('Failed to take measurement for %s',
                                      config.tag)
                        continue
                    values = [result.throughput for result in results]
                    self.output_perf_value(config.tag, values, units='Mbps',
                                           higher_is_better=True,
                                           graph=ap_config_tag)
                    result = netperf_runner.NetperfResult.from_samples(results)
                    self.write_perf_keyval(result.get_keyval(
                        prefix='_'.join([ap_config_tag, config.tag])))
            # Clean up router and client state for the next run.
            self.context.client.shill.disconnect(self.context.router.get_ssid())
            self.context.router.deconfig()
        end_time = time.time()
        logging.info('Running time %0.1f seconds.', end_time - start_time)
