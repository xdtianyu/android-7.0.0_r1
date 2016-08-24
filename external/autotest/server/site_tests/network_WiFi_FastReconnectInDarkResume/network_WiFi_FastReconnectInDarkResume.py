# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import time

from autotest_lib.client.common_lib import error
from autotest_lib.server.cros.network import hostap_config
from autotest_lib.server.cros.network import lucid_sleep_test_base
from autotest_lib.server.cros.network import wifi_client

class network_WiFi_FastReconnectInDarkResume(
        lucid_sleep_test_base.LucidSleepTestBase):
    """
    Test that we can reconnect quickly (within the span of one dark resume)
    if we are disconnected during suspend but the AP is still up.
    """

    version = 1

    def run_once(self):
        """Body of the test"""
        self.configure_and_connect_to_ap(hostap_config.HostapConfig(channel=1))
        client = self.context.client
        client_mac = client.wifi_mac
        router = self.context.router

        # Enable the dark connect feature in shill.
        with client.wake_on_wifi_features(wifi_client.WAKE_ON_WIFI_DARKCONNECT):
            logging.info('Set up WoWLAN')
            prev_dark_resume_count = self.dr_utils.count_dark_resumes()

            with self.dr_utils.suspend():
                # Wait for suspend actions to finish.
                time.sleep(wifi_client.SUSPEND_WAIT_TIME_SECONDS)

                logging.info('Deauthenticating the DUT')
                # A deauth packet should instantaneously disconnect the DUT
                # from the AP without bringing the AP down.
                router.deauth_client(client_mac)

                # Wait for the DUT to receive the disconnect, wake in
                # dark resume, reconnect, then suspend again.
                time.sleep(wifi_client.DISCONNECT_WAIT_TIME_SECONDS +
                           wifi_client.DARK_RESUME_WAIT_TIME_SECONDS)

            client.check_connected_on_last_resume()
            dark_resume_count = (self.dr_utils.count_dark_resumes() -
                                 prev_dark_resume_count)
            if dark_resume_count != 1:
                # If there was more than 1 dark resume, the DUT might not have
                # reconnected on the dark resume triggered by the disconnect.
                raise error.TestFail('Expected exactly one dark resume')
