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

class network_WiFi_ReconnectInDarkResume(
        lucid_sleep_test_base.LucidSleepTestBase):
    """Test that known WiFi access points wake up the system."""

    version = 1

    def run_once(self,
                 disconnect_before_suspend=False,
                 reconnect_to_same_ap=True,
                 num_iterations=1):
        """Body of the test

        @param disconnect_before_suspend: whether we disconnect the DUT before
        or after first suspending it.
        @param reconnect_to_same_ap: if this is true, during suspend, we bring
        up the same AP that the DUT was last connected to before the first
        suspend for the DUT to reconnect to.
        @param num_iterations: number of times to bring the AP down and up
        during dark resume. In each iteration, we bring the AP down once, and
        bring it up again once.

        """
        client = self.context.client
        router = self.context.router

        # We configure and connect to two APs (i.e. same AP configured with two
        # different SSIDs) so that the DUT has two preferred networks.
        first_ap_ssid = self.configure_and_connect_to_ap(
                hostap_config.HostapConfig(channel=1))
        router.deconfig_aps()
        second_ap_ssid = self.configure_and_connect_to_ap(
                hostap_config.HostapConfig(channel=1))

        if reconnect_to_same_ap:
            reconnect_ap_ssid = second_ap_ssid
        else:
            reconnect_ap_ssid = first_ap_ssid

        # Enable the dark connect feature in shill, and set the scan period.
        with contextlib.nested(
                client.wake_on_wifi_features(
                        wifi_client.WAKE_ON_WIFI_DARKCONNECT),
                client.net_detect_scan_period_seconds(
                        wifi_client.NET_DETECT_SCAN_WAIT_TIME_SECONDS)):
            logging.info('Set up WoWLAN')

            bring_ap_down_in_suspend = True
            if disconnect_before_suspend:
                # If we disconnect before suspend, we do not need to bring the
                # AP down again on the first suspend.
                bring_ap_down_in_suspend = False
                logging.info('Bringing AP %s down.' % router.get_ssid())
                router.deconfig_aps()
                time.sleep(wifi_client.DISCONNECT_WAIT_TIME_SECONDS)

            with self.dr_utils.suspend():
                for iter_num in xrange(1, num_iterations+1):
                    logging.info('Iteration %d of %d' %
                            (iter_num, num_iterations))
                    # Wait for suspend actions to finish.
                    time.sleep(wifi_client.SUSPEND_WAIT_TIME_SECONDS)

                    if bring_ap_down_in_suspend:
                        logging.info('Bringing AP %s down.' % router.get_ssid())
                        router.deconfig_aps()
                        # Wait for the DUT to receive the disconnect, wake in
                        # dark resume, then suspend again. Wait a little more
                        # after that so we don't trigger the next dark resume
                        # too soon and  set off the throttling mechanism.
                        time.sleep(wifi_client.DISCONNECT_WAIT_TIME_SECONDS +
                                   wifi_client.DARK_RESUME_WAIT_TIME_SECONDS +
                                   60)
                    else:
                        # We will bring the AP back up after this, so we
                        # will need to bring the AP down on any subsequent
                        # iterations to test wake on disconnect.
                        bring_ap_down_in_suspend = True

                    # Bring the AP back up to wake up the DUT.
                    logging.info('Bringing AP %s up.' % reconnect_ap_ssid)
                    self.context.configure(hostap_config.HostapConfig(
                            ssid=reconnect_ap_ssid, channel=1))

                    # Wait long enough for the NIC on the DUT to perform a net
                    # detect scan, discover the AP with the white-listed SSID,
                    # wake up in dark resume, connect, then suspend again.
                    time.sleep(wifi_client.NET_DETECT_SCAN_WAIT_TIME_SECONDS +
                               wifi_client.DARK_RESUME_WAIT_TIME_SECONDS)

            client.check_connected_on_last_resume()

            num_dark_resumes = self.dr_utils.count_dark_resumes()
            if disconnect_before_suspend and num_iterations == 1:
                # Only expect a single wake on SSID dark resume in this case
                # since no wake on disconnect would have been triggered.
                expected_num_dark_resumes = 1
            else:
                # Expect at least one disconnect dark resume and one SSID dark
                # resume per iteration.
                # Note: this is not foolproof; excess wakes on some iteration
                # can make up for a shortfall in dark resumes in another
                # iteration.
                expected_num_dark_resumes = 2 * num_iterations
            if num_dark_resumes < expected_num_dark_resumes:
                raise error.TestFail('Client only came up in %d dark resumes '
                                     'during the test (expected: at least %d)' %
                                     (num_dark_resumes,
                                      expected_num_dark_resumes))
