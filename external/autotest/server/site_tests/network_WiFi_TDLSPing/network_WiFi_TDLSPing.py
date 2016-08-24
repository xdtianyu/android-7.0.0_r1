# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import time

from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros.network import ping_runner
from autotest_lib.client.common_lib.cros.network import tcpdump_analyzer
from autotest_lib.client.common_lib.cros.network import xmlrpc_datatypes
from autotest_lib.server import site_linux_system
from autotest_lib.server.cros.network import hostap_config
from autotest_lib.server.cros.network import wifi_cell_test_base


class network_WiFi_TDLSPing(wifi_cell_test_base.WiFiCellTestBase):
    """Tests that the DUT can establish a TDLS link to a connected peer.

    This test associates the DUT with an AP, then attaches a peer
    client to the same AP.  After enabling a TDLS link from the DUT
    to the attached peer, we should see in the over-the-air packets
    that a ping between these devices does not use the AP as a relay
    any more.

    """

    version = 1

    def ping_and_check_for_tdls(self, frequency, expected):
        """
        Use an over-the-air packet capture to check whether we see
        ICMP packets from the DUT that indicate it is using a TDLS
        link to transmit its requests.  Raise an exception if this
        was not what was |expected|.

        @param frequency: int frequency on which to perform the packet capture.
        @param expected: bool set to true if we expect the sender to use TDLS.

        """
        self.context.router.start_capture(frequency)

        # Since we don't wait for the TDLS link to come up, it's possible
        # that we'll have some fairly drastic packet loss as the link is
        # being established.  We don't care about that in this test, except
        # that we should see at least a few packets by the end of the ping
        # we can use to test for TDLS.  Therefore we ignore the statistical
        # result of the ping.
        ping_config = ping_runner.PingConfig(
                self.context.router.local_peer_ip_address(0),
                ignore_result=True)
        self.context.assert_ping_from_dut(ping_config=ping_config)

        results = self.context.router.stop_capture()
        if len(results) != 1:
            raise error.TestError('Expected to generate one packet '
                                  'capture but got %d captures instead.' %
                                  len(results))
        pcap_result = results[0]

        logging.info('Analyzing packet capture...')

        # Filter for packets from the DUT.
        client_mac_filter = 'wlan.sa==%s' % self.context.client.wifi_mac

        # In this test we only care that the outgoing ICMP requests are
        # sent over IBSS, so we filter for ICMP echo requests explicitly.
        icmp_filter = 'icmp.type==0x08'

        # This filter requires a little explaining. DS status is the second byte
        # of the frame control field. This field contains the "tods" and
        # "fromds" bits in bit 0 and 1 respsectively. These bits have the
        # following interpretation:
        #
        #   ToDS  FromDS
        #     0     0      Ad-Hoc (IBSS)
        #     0     1      Traffic from client to the AP
        #     1     0      Traffic from AP to the client
        #     1     1      4-address mode for wireless distribution system
        #
        # TDLS co-opts the ToDS=0, FromDS=0 (IBSS) mode when transferring
        # data directly between peers. Therefore, to detect TDLS, we compare it
        # with 0.
        tdls_filter = 'wlan.fc.ds==0x00'

        dut_icmp_display_filter = ' and '.join(
                [client_mac_filter, icmp_filter, tdls_filter])
        frames = tcpdump_analyzer.get_frames(
                pcap_result.local_pcap_path,
                dut_icmp_display_filter,
                bad_fcs='include')
        if expected and not frames:
            raise error.TestFail('Packet capture did not contain any IBSS '
                                 'frames from the DUT!')
        elif not expected and frames:
            raise error.TestFail('Packet capture contains an IBSS frame '
                                 'from the DUT, but we did not expect them!')


    def run_once(self):
        """Test body."""
        client_caps = self.context.client.capabilities
        if site_linux_system.LinuxSystem.CAPABILITY_TDLS not in client_caps:
            raise error.TestNAError('DUT is incapable of TDLS')

        # Configure the AP.
        frequency = 2412
        self.context.configure(hostap_config.HostapConfig(
                frequency=frequency, force_wmm=True))
        router_ssid = self.context.router.get_ssid()

        # Connect the DUT to the AP.
        self.context.assert_connect_wifi(
                xmlrpc_datatypes.AssociationParameters(ssid=router_ssid))

        # Connect a client instance to the AP so the DUT has a peer to which
        # it can send TDLS traffic.
        self.context.router.add_connected_peer()

        # Test for TDLS connectivity to the peer, using the IP address.
        # We expect the first attempt to fail since the client does not
        # have the IP address of the peer in its ARP cache.
        peer_ip = self.context.router.local_peer_ip_address(0)
        link_state = self.context.client.query_tdls_link(peer_ip)
        if link_state is not False:
            raise error.TestError(
                    'First query of TDLS link succeeded: %r' % link_state)

        # Wait a reasonable time for the ARP triggered by the first TDLS
        # command to succeed.
        time.sleep(1)

        # A second attempt should succeed, since by now the ARP cache should
        # be populated.  However at this time there should be no link.
        link_state = self.context.client.query_tdls_link(peer_ip)
        if link_state != 'Nonexistent':
            raise error.TestError(
                    'DUT does not report a missing TDLS link: %r' % link_state)

        # Perform TDLS discover and check the status after waiting for response.
        self.context.client.discover_tdls_link(peer_ip)
        try:
            utils.poll_for_condition(
                    lambda: (self.context.client.query_tdls_link(peer_ip) ==
                    'Disconnected'), timeout=1)
        except utils.TimeoutError:
            link_state = self.context.client.query_tdls_link(peer_ip)
            logging.error('DUT does not report TDLS link is disconnected: %r',
                          link_state)

        # Ping from DUT to the associated peer without TDLS.
        self.ping_and_check_for_tdls(frequency, expected=False)

        # Ping from DUT to the associated peer with TDLS.
        self.context.client.establish_tdls_link(peer_ip)
        self.ping_and_check_for_tdls(frequency, expected=True)

        # Ensure that the DUT reports the TDLS link as being active.
        # Use the MAC address to ensure we can perform TDLS requests
        # against either IP or MAC addresses.
        peer_mac = self.context.router.local_peer_mac_address()
        link_state = self.context.client.query_tdls_link(peer_mac)
        if link_state != 'Connected':
            raise error.TestError(
                    'DUT does not report TDLS link is active: %r' % link_state)
