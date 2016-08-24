# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import random
import string
import time

from autotest_lib.server.cros.network import frame_sender
from autotest_lib.server.cros.network import hostap_config
from autotest_lib.server.cros.network import wifi_interface_claim_context
from autotest_lib.server import site_linux_system
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros.network import tcpdump_analyzer
from autotest_lib.server.cros.network import wifi_cell_test_base


class network_WiFi_ChannelScanDwellTime(wifi_cell_test_base.WiFiCellTestBase):
    """Test for determine channel scan dwell time."""
    version = 1

    KNOWN_TEST_PREFIX = 'network_WiFi'
    SUFFIX_LETTERS = string.ascii_lowercase + string.digits
    DELAY_INTERVAL_MILLISECONDS = 1
    SCAN_RETRY_TIMEOUT_SECONDS = 10
    NUM_BSS = 1024
    MISSING_BEACON_THRESHOLD = 2
    FREQUENCY_MHZ = 2412
    MSEC_PER_SEC = 1000

    def _build_ssid_prefix(self):
        """Build ssid prefix."""
        unique_salt = ''.join([random.choice(self.SUFFIX_LETTERS)
                               for x in range(5)])
        prefix = self.__class__.__name__[len(self.KNOWN_TEST_PREFIX):]
        prefix = prefix.lstrip('_')
        prefix += '_' + unique_salt + '_'
        return prefix[-23:]


    def _get_ssid_index(self, ssid):
        """Return the SSID index from an SSID string.

        Given an SSID of the form [testName]_[salt]_[index], returns |index|.

        @param ssid: full SSID, as received in scan results.
        @return int SSID index.
        """
        return int(ssid.split('_')[-1], 16)


    def _get_beacon_timestamp(self, beacon_frames, ssid_num):
        """Return the time at which the beacon with |ssid_num| was transmitted.

        If multiple beacons match |ssid_num|, return the time of the first
        matching beacon.

        @param beacon_frames: List of Frames.
        @param ssid_num: int SSID number to match.
        @return datetime time at which beacon was transmitted.
        """
        for frame in beacon_frames:
            if self._get_ssid_index(frame.ssid) == ssid_num:
                return frame.time_datetime
        else:
            raise error.TestFail('Failed to find SSID %d in pcap.' % ssid_num)


    def _get_dwell_time(self, bss_list, sent_beacon_frames):
        """Parse scan result to get dwell time.

        Calculate dwell time based on the SSIDs in the scan result.

        @param bss_list: List of BSSs.
        @param sent_beacon_frames: List of Frames, as captured on sender.

        @return int dwell time in ms.
        """
        ssid_index = [self._get_ssid_index(bss) for bss in bss_list]
        # Calculate dwell time based on the start ssid index and end ssid index.
        ssid_index.sort()
        index_diff = ssid_index[-1] - ssid_index[0]

        # Check if number of missed beacon frames exceed the test threshold.
        missed_beacons = index_diff - (len(ssid_index) - 1)
        if missed_beacons > self.MISSING_BEACON_THRESHOLD:
            logging.info('Missed %d beacon frames, SSID Index: %r',
                         missed_beacons, ssid_index)
            raise error.TestFail('DUT missed more than %d beacon frames' %
                                 missed_beacons)

        first_ssid_tstamp = self._get_beacon_timestamp(
            sent_beacon_frames, ssid_index[0])
        last_ssid_tstamp = self._get_beacon_timestamp(
            sent_beacon_frames, ssid_index[-1])
        return int(round(
            (last_ssid_tstamp - first_ssid_tstamp).total_seconds() *
            self.MSEC_PER_SEC))


    def _channel_dwell_time_test(self, single_channel):
        """Perform test to determine channel dwell time.

        This function invoke FrameSender to continuously send beacon frames
        for specific number of BSSs with specific delay, the SSIDs of the
        BSS are in hex numerical order. And at the same time, perform wifi scan
        on the DUT. The index in the SSIDs of the scan result will be used to
        interpret the relative start time and end time of the channel scan.

        @param single_channel: bool perform single channel scan if true.

        @return int dwell time in ms.

        """
        dwell_time = 0
        channel = hostap_config.HostapConfig.get_channel_for_frequency(
            self.FREQUENCY_MHZ)
        self.context.router.start_capture(self.FREQUENCY_MHZ)
        ssid_prefix = self._build_ssid_prefix()

        with frame_sender.FrameSender(self.context.router, 'beacon', channel,
                                      ssid_prefix=ssid_prefix,
                                      num_bss=self.NUM_BSS,
                                      frame_count=0,
                                      delay=self.DELAY_INTERVAL_MILLISECONDS):
            if single_channel:
                frequencies = [self.FREQUENCY_MHZ]
            else:
                frequencies = []
            # Perform scan
            start_time = time.time()
            while time.time() - start_time < self.SCAN_RETRY_TIMEOUT_SECONDS:
                bss_list = self.context.client.iw_runner.scan(
                        self.context.client.wifi_if, frequencies=frequencies)

                if bss_list is not None:
                    break

                time.sleep(0.5)
            else:
                raise error.TestFail('Unable to trigger scan on client.')
            if not bss_list:
                raise error.TestFail('Failed to find any BSS')

            # Remaining work is done outside the FrameSender
            # context. This is to ensure that no additional frames are
            # transmitted while we're waiting for the packet capture
            # to complete.

        pcap_path = self.context.router.stop_capture()[0].local_pcap_path

        # Filter scan result based on ssid prefix to remove any cached
        # BSSs from previous run.
        result_list = [bss.ssid for bss in bss_list if
                       bss.ssid.startswith(ssid_prefix)]
        if result_list is None:
            raise error.TestFail('Failed to find any BSS for this test')

        beacon_frames = tcpdump_analyzer.get_frames(
            pcap_path, tcpdump_analyzer.WLAN_BEACON_ACCEPTOR, bad_fcs='include')
        # Filter beacon frames based on ssid prefix.
        result_beacon_frames = [beacon_frame for beacon_frame in beacon_frames if
                                beacon_frame.ssid.startswith(ssid_prefix)]
        if result_beacon_frames is None:
            raise error.TestFail('Failed to find any beacons for this test')
        return self._get_dwell_time(result_list, result_beacon_frames)


    def run_once(self):
        if self.context.router.board == "panther":
            raise error.TestNAError('Panther router does not support manual '
                                    'beacon frame generation')
        self.context.router.require_capabilities(
                  [site_linux_system.LinuxSystem.
                          CAPABILITY_SEND_MANAGEMENT_FRAME])
        # Claim the control over the wifi interface from WiFiClient, which
        # will prevent shill and wpa_supplicant from managing that interface.
        # So this test can have the sole ownership of the interface and can
        # perform scans without interference from shill and wpa_supplicant.
        with wifi_interface_claim_context.WiFiInterfaceClaimContext(
                self.context.client):
            # Get channel dwell time for single-channel scan
            dwell_time = self._channel_dwell_time_test(True)
            logging.info('Channel dwell time for single-channel scan: %d ms',
                         dwell_time)
            self.write_perf_keyval(
                    {'dwell_time_single_channel_scan': dwell_time})
