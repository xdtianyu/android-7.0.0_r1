# Copyright (c) 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import time

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros.network import xmlrpc_datatypes
from autotest_lib.server import site_linux_system
from autotest_lib.server.cros.network import frame_sender
from autotest_lib.server.cros.network import hostap_config
from autotest_lib.server.cros.network import wifi_cell_test_base

class network_WiFi_MalformedProbeResp(wifi_cell_test_base.WiFiCellTestBase):
    """Test that we can stay connected to the configured AP when receiving
    malformed probe responses from an AP that we are not connected to."""
    version = 1

    PROBE_RESPONSE_DELAY_MSEC = 50
    SCAN_LOOP_SEC = 60
    SCAN_LOOP_SLEEP_SEC = 10
    PROBE_RESPONSE_TEST_CHANNEL = 1

    def run_once(self):
        """Sets up a router, connects to it, pings it, and repeats."""
        configuration = hostap_config.HostapConfig(
                channel=self.PROBE_RESPONSE_TEST_CHANNEL,
                mode=hostap_config.HostapConfig.MODE_11B)
        self.context.router.require_capabilities(
            [site_linux_system.LinuxSystem.CAPABILITY_SEND_MANAGEMENT_FRAME])

        self.context.configure(configuration)
        client_mac = self.context.client.wifi_mac

        pretest_reset_count = self.context.client.get_num_card_resets()
        logging.debug('pretest_reset_count=%d', pretest_reset_count)
        self.context.router.start_capture(
            configuration.frequency,
            ht_type=configuration.ht_packet_capture_mode)
        assoc_params = xmlrpc_datatypes.AssociationParameters()
        assoc_params.ssid = self.context.router.get_ssid()
        assoc_result = self.context.assert_connect_wifi(assoc_params)
        start_time = time.time()
        count = 1
        scan = 0
        rx_probe_resp_count = 0
        with self.context.client.assert_no_disconnects():
            with frame_sender.FrameSender(
                    self.context.router,
                    'probe_response',
                    self.PROBE_RESPONSE_TEST_CHANNEL,
                    ssid_prefix='TestingProbes',
                    num_bss=1,
                    frame_count=0,
                    delay=self.PROBE_RESPONSE_DELAY_MSEC,
                    dest_addr=client_mac,
                    probe_resp_footer='\xdd\xb7\x00\x1a\x11\x01\x01\x02\x03'):
                while time.time() - start_time < self.SCAN_LOOP_SEC:
                    bss_list = self.context.client.iw_runner.scan(
                            self.context.client.wifi_if, [2412])
                    for bss in bss_list:
                        logging.debug('found bss: %s', bss.ssid)
                        if bss.ssid == 'TestingProbes00000000':
                            rx_probe_resp_count += 1
                    time.sleep(self.SCAN_LOOP_SLEEP_SEC)
                else:
                    logging.debug('done scanning for networks')

        logging.debug('received %s probe_responses', rx_probe_resp_count)
        if rx_probe_resp_count == 0:
            raise error.TestFail('Client failed to receive probe responses')

        reset_count = self.context.client.get_num_card_resets()
        logging.debug('reset count = %s', reset_count)
        test_resets = reset_count - pretest_reset_count
        if test_resets < 0:
            logging.debug('logs rotated during test')
            if reset_count > 0:
                test_resets = reset_count

        if test_resets > 0:
            raise error.TestFail('Client reset card')
        self.context.client.shill.disconnect(assoc_params.ssid)
        self.context.router.deconfig()
        self.context.router.stop_capture()
