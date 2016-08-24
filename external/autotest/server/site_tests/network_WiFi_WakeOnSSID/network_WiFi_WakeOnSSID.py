# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import contextlib
import logging
import time

from autotest_lib.client.common_lib import error
from autotest_lib.server.cros.network import hostap_config
from autotest_lib.server.cros.network import lucid_sleep_test_base
from autotest_lib.server.cros.network import wifi_client

class network_WiFi_WakeOnSSID(lucid_sleep_test_base.LucidSleepTestBase):
    """Test that known WiFi access points wake up the system."""

    version = 1

    def run_once(self):
        """Body of the test."""
        ap_ssid = self.configure_and_connect_to_ap(
                hostap_config.HostapConfig(channel=1))
        client = self.context.client
        router = self.context.router

        # Enable the dark connect feature in shill, and set the scan period.
        with contextlib.nested(
                client.wake_on_wifi_features(
                        wifi_client.WAKE_ON_WIFI_DARKCONNECT),
                client.net_detect_scan_period_seconds(
                        wifi_client.NET_DETECT_SCAN_WAIT_TIME_SECONDS)):
            logging.info('Set up WoWLAN')

            # Bring the AP down so the DUT suspends disconnected.
            router.deconfig_aps()

            with self.dr_utils.suspend():
                # Wait for suspend actions and first scan to finish.
                time.sleep(wifi_client.SUSPEND_WAIT_TIME_SECONDS +
                           wifi_client.NET_DETECT_SCAN_WAIT_TIME_SECONDS)

                # Bring the AP back up to wake up the DUT.
                logging.info('Bringing AP back online.')
                self.context.configure(hostap_config.HostapConfig(
                        ssid=ap_ssid, channel=1))

                # Wait long enough for the NIC on the DUT to perform a net
                # detect scan, discover the AP with the white-listed SSID, wake
                # up in dark resume, then suspend again.
                time.sleep(wifi_client.NET_DETECT_SCAN_WAIT_TIME_SECONDS +
                           wifi_client.DARK_RESUME_WAIT_TIME_SECONDS)

                # Ensure that net detect did not trigger a full wake.
                if client.host.wait_up(
                        timeout=wifi_client.WAIT_UP_TIMEOUT_SECONDS):
                    raise error.TestFail('Client woke up fully.')

                if self.dr_utils.count_dark_resumes() < 1:
                    raise error.TestFail('Client failed to wake up.')

                logging.info('Client woke up successfully.')
