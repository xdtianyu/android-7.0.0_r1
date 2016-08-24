# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.cros import dhcp_packet
from autotest_lib.client.cros import dhcp_test_base

# Length of time the lease from the DHCP server is valid.
LEASE_TIME_SECONDS = 60
# We'll fill in the subnet and give this address to the client.
INTENDED_IP_SUFFIX = "0.0.0.101"

class network_DhcpNonAsciiParameter(dhcp_test_base.DhcpTestBase):
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
                "nyan.cat.google.com",
                "fail.whale.google.com",
                "zircon.encrusted.tweezers.google.com",
                ]
        # Set a server name that is invalid as ASCII or UTF-8.
        server_name = "\xff"
        # This is the pool of information the server will give out to the client
        # upon request.
        dhcp_options = {
                dhcp_packet.OPTION_SERVER_ID : self.server_ip,
                dhcp_packet.OPTION_SUBNET_MASK : subnet_mask,
                dhcp_packet.OPTION_IP_LEASE_TIME : LEASE_TIME_SECONDS,
                dhcp_packet.OPTION_REQUESTED_IP : intended_ip,
                dhcp_packet.OPTION_DNS_SERVERS : dns_servers,
                dhcp_packet.OPTION_DOMAIN_NAME : domain_name,
                dhcp_packet.OPTION_DNS_DOMAIN_SEARCH_LIST : dns_search_list
                }
        dhcp_fields = {
                dhcp_packet.FIELD_LEGACY_SERVER_NAME : server_name
                }
        self.negotiate_and_check_lease(dhcp_options, dhcp_fields)
