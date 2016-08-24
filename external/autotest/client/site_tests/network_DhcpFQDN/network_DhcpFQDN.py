# Copyright (c) 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros.network import interface
from autotest_lib.client.cros import dhcp_handling_rule
from autotest_lib.client.cros import dhcp_packet
from autotest_lib.client.cros import dhcp_test_base
from autotest_lib.client.cros.networking import shill_proxy

# Length of time the lease from the DHCP server is valid.
LEASE_TIME_SECONDS = 60
# We'll fill in the subnet and give this address to the client.
INTENDED_IP_SUFFIX = "0.0.0.101"
# We should be able to complete a DHCP negotiation in this amount of time.
DHCP_NEGOTIATION_TIMEOUT_SECONDS = 10

class network_DhcpFQDN(dhcp_test_base.DhcpTestBase):
    """Test implemenation of client completing negotiation with FQDN flag."""

    def test_body(self):
        """Main body of the test."""
        subnet_mask = self.ethernet_pair.interface_subnet_mask
        intended_ip = dhcp_test_base.DhcpTestBase.rewrite_ip_suffix(
                subnet_mask,
                self.server_ip,
                INTENDED_IP_SUFFIX)
        # It doesn't matter what is contained in this option value, except that
        # the DHCP client does not crash decoding it or passing its
        # interpretation of it back to shill.
        fqdn_option = '\x03\xff\x00'
        # This is the pool of information the server will give out to the client
        # upon request.
        dhcp_options = {
                dhcp_packet.OPTION_SERVER_ID : self.server_ip,
                dhcp_packet.OPTION_SUBNET_MASK : subnet_mask,
                dhcp_packet.OPTION_IP_LEASE_TIME : LEASE_TIME_SECONDS,
                dhcp_packet.OPTION_REQUESTED_IP : intended_ip,
                dhcp_packet.OPTION_FULLY_QUALIFIED_DOMAIN_NAME : fqdn_option
                }
        rules = [
                dhcp_handling_rule.DhcpHandlingRule_RespondToDiscovery(
                        intended_ip, self.server_ip, dhcp_options, {}),
                dhcp_handling_rule.DhcpHandlingRule_RespondToRequest(
                        intended_ip, self.server_ip, dhcp_options, {})
                ]
        rules[-1].is_final_handler = True

        # In some DHCP server implementations, the FQDN option is provided in
        # the DHCP ACK response without the client requesting it.
        rules[-1].force_reply_options = [
                dhcp_packet.OPTION_FULLY_QUALIFIED_DOMAIN_NAME ]

        self.server.start_test(rules, DHCP_NEGOTIATION_TIMEOUT_SECONDS)
        self.server.wait_for_test_to_finish()
        if not self.server.last_test_passed:
            raise error.TestFail('Test server didn\'t get all the messages it '
                                 'was told to expect during negotiation.')

        # This test passes if the DHCP client lives long enough to send
        # the network configuration to shill.
        self.wait_for_dhcp_propagation()
        self.check_dhcp_config(dhcp_options)
