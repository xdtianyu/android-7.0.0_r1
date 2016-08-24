# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import time

from autotest_lib.client.common_lib import error
from autotest_lib.server.cros.network import hostap_config
from autotest_lib.server.cros.network import lucid_sleep_test_base
from autotest_lib.server.cros.network import wifi_client

class network_WiFi_WoWLAN(lucid_sleep_test_base.LucidSleepTestBase):
    """Test that WiFi packets can wake up the system."""

    version = 1

    def run_once(self):
        """Body of the test."""
        self.configure_and_connect_to_ap(hostap_config.HostapConfig(channel=1))
        self.context.assert_ping_from_dut()

        client = self.context.client
        router = self.context.router
        dut_mac = client.wifi_mac
        dut_ip = client.wifi_ip

        logging.info('DUT WiFi MAC = %s, IPv4 = %s', dut_mac, dut_ip)
        logging.info('Router WiFi IPv4 = %s', router.wifi_ip)

        # Set up WoWLAN to wake on packets and register ip, then sleep
        with client.wake_on_wifi_features(wifi_client.WAKE_ON_WIFI_PACKET):
            logging.info('Set up WoWLAN')
            client.add_wake_packet_source(router.wifi_ip)

            with self.dr_utils.suspend():
                time.sleep(wifi_client.SUSPEND_WAIT_TIME_SECONDS)

                router.send_magic_packet(dut_ip, dut_mac)

                # Wait for the DUT to wake up in dark resume and suspend again.
                time.sleep(wifi_client.RECEIVE_PACKET_WAIT_TIME_SECONDS +
                           wifi_client.DARK_RESUME_WAIT_TIME_SECONDS)

                # Ensure that wake on packet did not trigger a full wake.
                if client.host.wait_up(
                        timeout=wifi_client.WAIT_UP_TIMEOUT_SECONDS):
                    raise error.TestFail('Client woke up fully.')

                if self.dr_utils.count_dark_resumes() < 1:
                    raise error.TestFail('Client failed to wake up.')

                logging.info('Client woke up successfully.')


    def cleanup(self):
        self.context.client.remove_all_wake_packet_sources()
        super(network_WiFi_WoWLAN, self).cleanup()
