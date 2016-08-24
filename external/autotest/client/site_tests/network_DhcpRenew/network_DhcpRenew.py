# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import time

from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import dhcp_handling_rule
from autotest_lib.client.cros import dhcp_packet
from autotest_lib.client.cros import dhcp_test_base

# dhcpcd has a 20 second minimal accepted lease time
LEASE_TIME_SECONDS = 20
# dhcpcd should request a renewal after this many seconds.
LEASE_T1_TIME = 10
# dhcpcd will broadcast a REQUEST after this many seconds.
LEASE_T2_TIME = 15
# We had better have lost the lease 25 seconds after we gained it.
DHCP_RENEWAL_TIMEOUT_SECONDS = 25
# We'll fill in the subnet and give this address to the client.
INTENDED_IP_SUFFIX = "0.0.0.101"
# How far off the expected deadlines we'll accept the T1/T2 packets.
RENEWAL_TIME_DELTA_SECONDS = 2.0
# Time by which we are sure shill will give up on the DHCP client.
DHCP_ATTEMPT_TIMEOUT_SECONDS = 40

class network_DhcpRenew(dhcp_test_base.DhcpTestBase):
    """Tests DHCP renewal process in the connection manager."""
    def test_body(self):
        subnet_mask = self.ethernet_pair.interface_subnet_mask
        intended_ip = dhcp_test_base.DhcpTestBase.rewrite_ip_suffix(
                subnet_mask,
                self.server_ip,
                INTENDED_IP_SUFFIX)
        # Two real name servers, and a bogus one to be unpredictable.
        dns_servers = ["8.8.8.8", "8.8.4.4", "192.168.87.88"]
        domain_name = "corp.google.com"
        dns_search_list = [
                "corgie.google.com",
                "lies.google.com",
                "that.is.a.tasty.burger.google.com",
                ]
        # This is the pool of information the server will give out to the client
        # upon request.
        dhcp_options = {
                dhcp_packet.OPTION_SERVER_ID : self.server_ip,
                dhcp_packet.OPTION_SUBNET_MASK : subnet_mask,
                dhcp_packet.OPTION_IP_LEASE_TIME : LEASE_TIME_SECONDS,
                dhcp_packet.OPTION_REQUESTED_IP : intended_ip,
                dhcp_packet.OPTION_DNS_SERVERS : dns_servers,
                dhcp_packet.OPTION_DOMAIN_NAME : domain_name,
                dhcp_packet.OPTION_DNS_DOMAIN_SEARCH_LIST : dns_search_list,
                dhcp_packet.OPTION_RENEWAL_T1_TIME_VALUE : LEASE_T1_TIME,
                dhcp_packet.OPTION_REBINDING_T2_TIME_VALUE : LEASE_T2_TIME,
                }
        self.negotiate_and_check_lease(dhcp_options)
        # This is very imprecise, since there is some built in delay in
        # negotiate_new_lease() for settings propagations, but we're not
        # interested in microsecond timings anyway.
        lease_start_time = time.time()
        t1_deadline = lease_start_time + LEASE_T1_TIME
        t2_deadline = lease_start_time + LEASE_T2_TIME
        # Ignore the T1 deadline packet.
        t1_handler = dhcp_handling_rule.DhcpHandlingRule_RespondToRequest(
                intended_ip,
                self.server_ip,
                dhcp_options,
                {},
                should_respond=False)
        t1_handler.target_time_seconds = t1_deadline
        t1_handler.allowable_time_delta_seconds = RENEWAL_TIME_DELTA_SECONDS
        t2_handler = dhcp_handling_rule.DhcpHandlingRule_RespondToPostT2Request(
                intended_ip,
                self.server_ip,
                dhcp_options,
                {},
                should_respond=False)
        t2_handler.target_time_seconds = t2_deadline
        t2_handler.allowable_time_delta_seconds = RENEWAL_TIME_DELTA_SECONDS
        discovery_handler = \
                dhcp_handling_rule.DhcpHandlingRule_RespondToDiscovery(
                        intended_ip,
                        self.server_ip,
                        dhcp_options,
                        {},
                        should_respond=False)
        rules = [t1_handler, t2_handler, discovery_handler]
        rules[-1].is_final_handler = True
        self.server.start_test(rules, DHCP_RENEWAL_TIMEOUT_SECONDS)
        self.server.wait_for_test_to_finish()
        if not self.server.last_test_passed:
            raise error.TestFail("Test server didn't get all the messages it "
                                 "was told to expect for renewal.")

        # The service should leave the connected state after shill attempts
        # one last DHCP attempt from scratch.  We may miss the transition to the
        # "idle" state since the system immediately attempts to re-connect, so
        # we also test for the "configuration" state.
        service = self.find_ethernet_service(
                self.ethernet_pair.peer_interface_name)
        (successful, state, duration) = self.shill_proxy.wait_for_property_in(
                service,
                self.shill_proxy.SERVICE_PROPERTY_STATE,
                ('failure', 'idle', 'configuration'),
                DHCP_ATTEMPT_TIMEOUT_SECONDS)
        if not successful:
            raise error.TestFail('Service failed to go idle in %ds (state %s)' %
                                 (duration, state))
        logging.info('In state "%s" after %d seconds', state, duration)
