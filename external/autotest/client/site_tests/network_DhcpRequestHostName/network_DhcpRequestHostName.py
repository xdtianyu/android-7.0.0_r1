# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import utils

from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import dhcp_packet
from autotest_lib.client.cros import dhcp_test_base

# Length of time the lease from the DHCP server is valid.
LEASE_TIME_SECONDS = 60
# We'll fill in the subnet and give this address to the client.
INTENDED_IP_SUFFIX = '0.0.0.101'
# Most ChromeOS devices are configured with this name.
DEFAULT_HOSTNAME = 'localhost'
# Hostname we'll provide to the device.
TEST_HOSTNAME = 'britney-spears'

class network_DhcpRequestHostName(dhcp_test_base.DhcpTestBase):
    """Tests that we can supply a hostname to the shill over DHCP."""
    def test_body(self):
        # Make sure that shill is started with
        # --accept-hostname-from=pseudoethernet0.
        required_flag = '--accept-hostname-from=pseudoethernet0'
        pid = utils.system_output('pgrep shill')
        process_info = utils.system_output('ps %s' % pid)
        if required_flag not in process_info:
            raise error.TestNAError('Invalid Test. '
                                    'Expected shill to be started with %s' %
                                    required_flag)

        # Keep track of the original hostname.
        original_hostname = utils.system_output('hostname')
        if original_hostname != DEFAULT_HOSTNAME:
            logging.warning('Unexpected starting hostname %s (expected %s)',
                            original_hostname, DEFAULT_HOSTNAME)
            # Set the hostname to something we know.
            utils.system('hostname %s' % DEFAULT_HOSTNAME)

        subnet_mask = self.ethernet_pair.interface_subnet_mask
        intended_ip = dhcp_test_base.DhcpTestBase.rewrite_ip_suffix(
                subnet_mask,
                self.server_ip,
                INTENDED_IP_SUFFIX)
        # Two real name servers, and a bogus one to be unpredictable.
        dns_servers = ['8.8.8.8', '8.8.4.4', '192.168.87.88']
        domain_name = 'corp.google.com'
        dns_search_list = [
                'corgie.google.com',
                'lies.google.com',
                'that.is.a.tasty.burger.google.com',
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
                dhcp_packet.OPTION_HOST_NAME : TEST_HOSTNAME,
                dhcp_packet.OPTION_DNS_DOMAIN_SEARCH_LIST : dns_search_list,
                }

        try:
            self.negotiate_and_check_lease(dhcp_options)
            system_hostname = utils.system_output('hostname')
        finally:
            # Set the hostname back to the original to avoid side effects.
            utils.system_output('hostname %s' % original_hostname)

        # Test that shill updated the system hostname correctly.
        if system_hostname != TEST_HOSTNAME:
            raise error.TestFail('Expected system host name to be set to '
                                 '%s, but got %s instead.' %
                                 (TEST_HOSTNAME, system_hostname))
