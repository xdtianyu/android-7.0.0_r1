# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import utils
from autotest_lib.client.common_lib.cros.network import iw_runner
from autotest_lib.client.common_lib.cros.network import tcpdump_analyzer
from autotest_lib.client.common_lib.cros.network import xmlrpc_datatypes
from autotest_lib.server.cros.network import hostap_config
from autotest_lib.server.cros.network import netperf_runner
from autotest_lib.server.cros.network import wifi_cell_test_base


class network_WiFi_RateControl(wifi_cell_test_base.WiFiCellTestBase):
    """
    Test maximal achievable bandwidth on several channels per band.

    Conducts a performance test for a set of specified router configurations
    and reports results as keyval pairs.

    """

    version = 1

    # We only care about the encoding/MCS index of the frames, not the
    # contents.  Large snaplens fill up /tmp/ and make shuffling the bits
    # around take longer.  However, we might be interested in the contents of
    # the frames in the association process, and packets I've seen with HT IEs
    # seem to come in around 300 bytes.
    TEST_SNAPLEN = 400


    def parse_additional_arguments(self, commandline_args, additional_params):
        """
        Hook into super class to take control files parameters.

        @param commandline_args: dict of parsed parameters from the autotest.
        @param additional_params: list of HostapConfig objects.

        """
        self._ap_configs = additional_params


    def get_highest_mcs_rate(self, frequency):
        """
        Get the highest MCS index supported by the DUT on |frequency|.

        @param frequency: int frequency to look for supported MCS rates.
        @return int highest rate supported.

        """
        # Figure out the highest MCS index supported by this hardware.
        phys = iw_runner.IwRunner(
                remote_host=self.context.client.host).list_phys()
        if len(phys) != 1:
            raise error.TestFail('Test expects a single PHY, but we got %d' %
                                 len(phys))

        phy = phys[0]
        bands = [band for band in phy.bands if frequency in band.frequencies]
        if len(bands) != 1:
            raise error.TestFail('Test expects a single possible band for a '
                                 'given frequency, but this device has %d '
                                 'such bands.' % len(bands))

        # 32 is a special low throughput, high resilience mode.  Ignore it.
        possible_indices = filter(lambda x: x != 32, bands[0].mcs_indices)

        if not possible_indices:
            raise error.TestFail('No possible MCS indices on frequency %d' %
                                 frequency)

        return max(possible_indices)


    def check_bitrates_in_capture(self, pcap_result, max_mcs_index):
        """
        Check that frames in a packet capture have expected MCS indices.

        @param pcap_result: RemoteCaptureResult tuple.
        @param max_mcs_index: int MCS index representing the highest possible
                bitrate on this device.

        """
        logging.info('Analyzing packet capture...')
        display_filter = 'udp and ip.src==%s' % self.context.client.wifi_ip
        frames = tcpdump_analyzer.get_frames(
                pcap_result.local_pcap_path,
                display_filter,
                bad_fcs='include')

        logging.info('Grouping frames by MCS index')
        counts = {}
        for frame in frames:
            counts[frame.mcs_index] = counts.get(frame.mcs_index, 0) + 1
        logging.info('Saw WiFi frames with MCS indices: %r', counts)

        # Now figure out the index which the device sent the most packets with.
        dominant_index = None
        num_packets_sent = -1
        for index, num_packets in counts.iteritems():
            if num_packets > num_packets_sent:
                dominant_index = index
                num_packets_sent = num_packets

        # We should see that the device sent more frames with the maximal index
        # than anything else.  This checks that the rate controller is fairly
        # aggressive and using all of the device's capabilities.
        if dominant_index != max_mcs_index:
            raise error.TestFail('Failed to use best possible MCS '
                                 'index %d in a clean RF environment: %r' %
                                 (max_mcs_index, counts))


    def run_once(self):
        """Test body."""
        if utils.host_could_be_in_afe(self.context.client.host.hostname):
            # Just abort the test if we're in the lab and not on a
            # machine known to be conducted. The performance
            # requirements of this test are hard to meet, without
            # strong multi-path effects. (Our conducted setups are
            # designed to provide strong multi-path.)
            if not self.context.client.conductive:
                raise error.TestNAError(
                    'This test requires a great RF environment.')
        else:
            logging.error('Unable to determine if DUT has conducted '
                          'connection to AP. Treat any TestFail with '
                          'skepticism.')

        caps = [hostap_config.HostapConfig.N_CAPABILITY_GREENFIELD,
                hostap_config.HostapConfig.N_CAPABILITY_HT40]
        mode_11n = hostap_config.HostapConfig.MODE_11N_PURE
        get_config = lambda channel: hostap_config.HostapConfig(
                channel=channel, mode=mode_11n, n_capabilities=caps)
        netperf_config = netperf_runner.NetperfConfig(
                netperf_runner.NetperfConfig.TEST_TYPE_UDP_STREAM)
        for i, ap_config in enumerate([get_config(1), get_config(157)]):
            # Set up the router and associate the client with it.
            self.context.configure(ap_config)
            self.context.router.start_capture(
                    ap_config.frequency,
                    ht_type=ap_config.ht_packet_capture_mode,
                    snaplen=self.TEST_SNAPLEN)
            assoc_params = xmlrpc_datatypes.AssociationParameters(
                    ssid=self.context.router.get_ssid())
            self.context.assert_connect_wifi(assoc_params)
            with netperf_runner.NetperfRunner(self.context.client,
                                              self.context.router,
                                              netperf_config) as runner:
                runner.run()
            results = self.context.router.stop_capture()
            if len(results) != 1:
                raise error.TestError('Expected to generate one packet '
                                      'capture but got %d instead.' %
                                      len(results))

            # The device should sense that it is in a clean RF environment and
            # use the highest index to achieve maximal throughput.
            max_mcs_index = self.get_highest_mcs_rate(ap_config.frequency)
            self.check_bitrates_in_capture(results[0], max_mcs_index)
            # Clean up router and client state for the next run.
            self.context.client.shill.disconnect(self.context.router.get_ssid())
            self.context.router.deconfig()
