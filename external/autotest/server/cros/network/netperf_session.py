# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.server.cros.network import netperf_runner

class NetperfSession(object):
    """Abstracts a network performance measurement to reduce variance."""

    MAX_DEVIATION_FRACTION = 0.03
    MEASUREMENT_MAX_SAMPLES = 10
    MEASUREMENT_MIN_SAMPLES = 3
    WARMUP_SAMPLE_TIME_SECONDS = 2
    WARMUP_WINDOW_SIZE = 2
    WARMUP_MAX_SAMPLES = 10


    @staticmethod
    def _from_samples(samples):
        """Construct a NetperfResult that averages previous results.

        This makes typing this considerably easier.

        @param samples: list of NetperfResult
        @return NetperfResult average of samples.

        """
        return netperf_runner.NetperfResult.from_samples(samples)


    def __init__(self, client_proxy, server_proxy, ignore_failures=False):
        """Construct a NetperfSession.

        @param client_proxy: WiFiClient object.
        @param server_proxy: LinuxSystem object.

        """
        self._client_proxy = client_proxy
        self._server_proxy = server_proxy
        self._ignore_failures = ignore_failures


    def warmup_wifi_part(self, warmup_client=True):
        """Warm up a rate controller on the client or server.

        WiFi "warms up" in that rate controllers dynamically adjust to
        environmental conditions by increasing symbol rates until loss is
        observed.  This manifests as initially slow data transfer rates that
        get better over time.

        We'll say that a rate controller is warmed up if a small sample of
        WARMUP_WINDOW_SIZE throughput measurements has an average throughput
        within a standard deviation of the previous WARMUP_WINDOW_SIZE samples.

        @param warmup_client: bool True iff we should warmup the client rate
                controller.  Otherwise we warm up the server rate controller.

        """
        if warmup_client:
            # We say a station is warm if the TX throughput is maximized.
            # Each station only controls its own transmission TX rate.
            logging.info('Warming up the client WiFi rate controller.')
            test_type = netperf_runner.NetperfConfig.TEST_TYPE_TCP_STREAM
        else:
            logging.info('Warming up the server WiFi rate controller.')
            test_type = netperf_runner.NetperfConfig.TEST_TYPE_TCP_MAERTS
        config = netperf_runner.NetperfConfig(
                test_type, test_time=self.WARMUP_SAMPLE_TIME_SECONDS)
        warmup_history = []
        with netperf_runner.NetperfRunner(
                self._client_proxy, self._server_proxy, config) as runner:
            while len(warmup_history) < self.WARMUP_MAX_SAMPLES:
                warmup_history.append(runner.run())
                if len(warmup_history) > 2 * self.WARMUP_WINDOW_SIZE:
                    # Grab 2 * WARMUP_WINDOW_SIZE samples, divided into the most
                    # recent chunk and the chunk before that.
                    start = -2 * self.WARMUP_WINDOW_SIZE
                    middle = -self.WARMUP_WINDOW_SIZE
                    past_result = self._from_samples(
                            warmup_history[start:middle])
                    recent_result = self._from_samples(warmup_history[middle:])
                    if recent_result.throughput < (past_result.throughput +
                                                   past_result.throughput_dev):
                        logging.info('Rate controller is warmed.')
                        return
            else:
                logging.warning('Did not completely warmup the WiFi part.')


    def warmup_stations(self):
        """Warms up both the client and server stations."""
        self.warmup_wifi_part(warmup_client=True)
        self.warmup_wifi_part(warmup_client=False)


    def run(self, config):
        """Measure the average and standard deviation of a netperf test.

        @param config: NetperfConfig object.

        """
        logging.info('Performing %s measurements in netperf session.',
                     config.human_readable_tag)
        history = []
        none_count = 0
        final_result = None
        with netperf_runner.NetperfRunner(
                self._client_proxy, self._server_proxy, config) as runner:
            while len(history) + none_count < self.MEASUREMENT_MAX_SAMPLES:
                result = runner.run(ignore_failures=self._ignore_failures)
                if result is None:
                    none_count += 1
                    continue

                history.append(result)
                if len(history) < self.MEASUREMENT_MIN_SAMPLES:
                    continue

                final_result = self._from_samples(history)
                if final_result.all_deviations_less_than_fraction(
                        self.MAX_DEVIATION_FRACTION):
                    break

        if final_result is None:
            final_result = self._from_samples(history)
        logging.info('Took averaged measurement %r.', final_result)
        return history or None
