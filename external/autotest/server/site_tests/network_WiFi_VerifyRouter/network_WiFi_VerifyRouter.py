# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import time

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros.network import xmlrpc_datatypes
from autotest_lib.server import site_linux_system
from autotest_lib.server.cros.network import hostap_config
from autotest_lib.server.cros.network import wifi_cell_test_base


class network_WiFi_VerifyRouter(wifi_cell_test_base.WiFiCellTestBase):
    """Test that a dual radio router can use both radios."""
    version = 1
    MAX_ASSOCIATION_RETRIES = 8  # Super lucky number.  Not science.


    def _connect(self, wifi_params):
        assoc_result = xmlrpc_datatypes.deserialize(
                self.context.client.shill.connect_wifi(wifi_params))
        logging.info('Finished connection attempt to %s with times: '
                     'discovery=%.2f, association=%.2f, configuration=%.2f.',
                     wifi_params.ssid,
                     assoc_result.discovery_time,
                     assoc_result.association_time,
                     assoc_result.configuration_time)
        return assoc_result.success


    def _antenna_test(self, bitmap, channel):
        """Test that we can connect on |channel|, with given antenna |bitmap|.

        Sets up two radios on |channel|, configures both radios with the
        given antenna |bitmap|, and then verifies that a client can connect
        to the AP on each radio.

        Why do we run the two radios concurrently, instead of iterating over
        them? That's simply because our lower-layer code doesn't provide an
        interface for specifiying which PHY to run an AP on.

        To work around the API limitaiton, we bring up multiple APs, and let
        the lower-layer code spread them across radios. For stumpy/panther,
        this works in an obvious way. That is, each call to this method
        exercises phy0 and phy1.

        For whirlwind, we still cover all radios, but in a less obvious way.
        Calls with a 2.4 GHz channel exercise phy0 and phy2, while calls
        with a 5 GHz channel exercise phy1 and phy2.

        @param bitmap: int bitmask controlling which antennas to enable.
        @param channel: int Wifi channel to conduct test on

        """
        # Antenna can only be configured when the wireless interface is down.
        self.context.router.deconfig()
        # Set the bitmasks to both antennas on before turning one off.
        self.context.router.disable_antennas_except(3)
        # This seems to increase the probability that our association
        # attempts pass.  It is the very definition of a dark incantation.
        time.sleep(5)
        if bitmap != 3:
            self.context.router.disable_antennas_except(bitmap)
        # Setup two APs on |channel|. configure() will spread these across
        # radios.
        n_mode = hostap_config.HostapConfig.MODE_11N_MIXED
        ap_config = hostap_config.HostapConfig(channel=channel, mode=n_mode)
        self.context.configure(ap_config)
        self.context.configure(ap_config, multi_interface=True)
        failures = []
        # Verify connectivity to both APs. As the APs are spread
        # across radios, this exercises multiple radios.
        for instance in range(2):
            context_message = ('bitmap=%d, ap_instance=%d, channel=%d' %
                               (bitmap, instance, channel))
            logging.info('Connecting to AP with settings %s.',
                         context_message)
            client_conf = xmlrpc_datatypes.AssociationParameters(
                    ssid=self.context.router.get_ssid(instance=instance))
            if self._connect(client_conf):
                signal_level = self.context.client.wifi_signal_level
                logging.info('Signal level for AP %d with bitmap %d is %d',
                             instance, bitmap, signal_level)
                self.write_perf_keyval(
                        {'signal_for_ap_%d_bm_%d_ch_%d' %
                                 (instance, bitmap, channel):
                         signal_level})
            else:
                failures.append(context_message)
            # Don't automatically reconnect to this AP.
            self.context.client.shill.disconnect(
                    self.context.router.get_ssid(instance=instance))
        return failures


    def cleanup(self):
        """Clean up after the test is completed

        Perform additional cleanups after the test, the important thing is
        to re-enable all antennas.
        """
        self.context.router.deconfig()
        self.context.router.enable_all_antennas()
        super(network_WiFi_VerifyRouter, self).cleanup()


    def run_once(self):
        """Verify that all radios on this router are functional."""
        self.context.router.require_capabilities(
                [site_linux_system.LinuxSystem.CAPABILITY_MULTI_AP_SAME_BAND])

        all_failures = []
        # Run antenna test for 2GHz band and 5GHz band
        for channel in (6, 149):
            # First connect with both antennas enabled. Then connect with just
            # one antenna enabled at a time.
            for bitmap in (3, 1, 2):
                failures = set()
                for attempt in range(self.MAX_ASSOCIATION_RETRIES):
                    new_failures = self._antenna_test(bitmap, channel)
                    if not new_failures:
                        break
                    failures.update(new_failures)
                else:
                    all_failures += failures

        if all_failures:
            failure_message = ', '.join(
                    ['(' + message + ')' for message in all_failures])
            raise error.TestFail('Failed to connect when %s.' % failure_message)
