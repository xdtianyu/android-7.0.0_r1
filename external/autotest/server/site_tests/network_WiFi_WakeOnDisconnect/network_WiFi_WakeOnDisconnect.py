# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import time

from autotest_lib.client.common_lib import error
from autotest_lib.server.cros.network import hostap_config
from autotest_lib.server.cros.network import lucid_sleep_test_base
from autotest_lib.server.cros.network import wifi_client

class network_WiFi_WakeOnDisconnect(lucid_sleep_test_base.LucidSleepTestBase):
    """Test that WiFi disconnect wakes up the system."""

    version = 1

    def run_once(self):
        """Body of the test."""
        self.configure_and_connect_to_ap(hostap_config.HostapConfig(channel=1))
        client = self.context.client
        router = self.context.router

        # Enable the dark connect feature in shill.
        with client.wake_on_wifi_features(wifi_client.WAKE_ON_WIFI_DARKCONNECT):
            logging.info('Set up WoWLAN')

            with self.dr_utils.suspend():
                time.sleep(wifi_client.SUSPEND_WAIT_TIME_SECONDS)

                # Kick over the router to trigger wake on disconnect.
                router.deconfig_aps(silent=True)

                # Wait for the DUT to wake up in dark resume and suspend again.
                time.sleep(wifi_client.DARK_RESUME_WAIT_TIME_SECONDS)

                # Ensure that wake on packet did not trigger a full wake.
                if client.host.wait_up(
                        timeout=wifi_client.WAIT_UP_TIMEOUT_SECONDS):
                    raise error.TestFail('Client woke up fully.')

                if self.dr_utils.count_dark_resumes() < 1:
                    raise error.TestFail('Client failed to wake up.')

                logging.info('Client woke up successfully.')
