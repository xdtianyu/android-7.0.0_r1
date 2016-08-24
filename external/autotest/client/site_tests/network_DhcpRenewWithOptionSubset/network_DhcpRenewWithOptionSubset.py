# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import dhcp_handling_rule
from autotest_lib.client.cros import dhcp_packet
from autotest_lib.client.cros import dhcp_test_base

# dhcpcd has a 20 second minimal accepted lease time
LEASE_TIME_SECONDS = 20
# We'll fill in the subnet and give this address to the client.
INTENDED_IP_SUFFIX = "0.0.0.101"

class network_DhcpRenewWithOptionSubset(dhcp_test_base.DhcpTestBase):
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
        minimal_options = {
                dhcp_packet.OPTION_SERVER_ID : self.server_ip,
                dhcp_packet.OPTION_SUBNET_MASK : subnet_mask,
                dhcp_packet.OPTION_IP_LEASE_TIME : LEASE_TIME_SECONDS,
                dhcp_packet.OPTION_REQUESTED_IP : intended_ip,
                dhcp_packet.OPTION_DNS_SERVERS : dns_servers,
        }
        dhcp_options = minimal_options.copy()
        dhcp_options.update({
                dhcp_packet.OPTION_DOMAIN_NAME : domain_name,
                dhcp_packet.OPTION_DNS_DOMAIN_SEARCH_LIST : dns_search_list,
                })
        self.negotiate_and_check_lease(dhcp_options)

        # At renewal time, respond without the search list, and with a
        # different domain name from the original lease.
        changed_options = {
                dhcp_packet.OPTION_DOMAIN_NAME : "mail.google.com",
        }
        renew_options = minimal_options.copy()
        renew_options.update(changed_options)
        rules = [
                dhcp_handling_rule.DhcpHandlingRule_RespondToRequest(
                        intended_ip,
                        self.server_ip,
                        renew_options,
                        {},
                        should_respond=True,
                        # Per RFC-2131, the server identifier must be false
                        # during REBOOT.
                        expect_server_ip_set=False)
                ]
        rules[-1].is_final_handler = True
        self.server.start_test(
                rules, dhcp_test_base.DHCP_NEGOTIATION_TIMEOUT_SECONDS)

        # Trigger lease renewal on the client.
        interface_name = self.ethernet_pair.peer_interface_name
        self.get_interface_ipconfig_objects(interface_name)[0].Refresh()

        self.server.wait_for_test_to_finish()
        if not self.server.last_test_passed:
            raise error.TestFail("Test server didn't get a renewal request.")

        # Check to make sure the system retained the search list from the
        # initial lease, but also has the domain name from the ACK of the
        # DHCPREQUEST.
        dhcp_options.update(changed_options)
        self.check_dhcp_config(dhcp_options)
