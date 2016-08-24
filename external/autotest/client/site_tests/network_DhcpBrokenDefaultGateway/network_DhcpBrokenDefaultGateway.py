# Copyright 2015 The Chromium OS Authors. All rights reserved.
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
INTENDED_IP_SUFFIX = '0.0.0.101'
# We should be able to complete a DHCP negotiation in this amount of time.
DHCP_NEGOTIATION_TIMEOUT_SECONDS = 10

class network_DhcpBrokenDefaultGateway(dhcp_test_base.DhcpTestBase):
    """Test application of broken gateway route from DHCP server."""

    def check_shill_gateway_setup(self, interface_name, gateway_ip):
        """Check that the ipconfig in the client shows the gateway IP.

        @param interface_name string client network interface name.
        @param gateway_ip string expected gateway IP address.

        """
        proxy = shill_proxy.ShillProxy()
        device = proxy.find_object('Device', {'Name': interface_name})
        if device is None:
            raise error.TestFail('Device was not found.')
        device_properties = device.GetProperties(utf8_strings=True)
        ipconfig_path = device_properties['IPConfigs'][0]
        ipconfig = proxy.get_dbus_object('org.chromium.flimflam.IPConfig',
                                         ipconfig_path)
        ipconfig_properties = ipconfig.GetProperties(utf8_strings=True)
        ipconfig_gateway = ipconfig_properties['Gateway']
        if ipconfig_gateway != gateway_ip:
            raise error.TestFail('Shill gateway %s does '
                                 'not match expected %s.' %
                                 (ipconfig_gateway, gateway_ip))


    def check_routing_table_gateway_setup(self, interface_name, gateway_ip):
        """Check that the routing table in the client shows the gateway IP.

        @param interface_name string client network interface name.
        @param gateway_ip string expected gateway IP address.

        """
        default_route = interface.get_prioritized_default_route(
            host=None, interface_name_regex=interface_name)
        if not default_route:
            raise error.TestFail('No default route found.')
        if default_route.gateway != gateway_ip:
            raise error.TestFail('Routing table gateway %s does '
                                 'not match expected %s.' %
                                 (default_route.gateway, gateway_ip))


    def test_body(self):
        """Main body of the test."""
        subnet_mask = self.ethernet_pair.interface_subnet_mask
        intended_ip = dhcp_test_base.DhcpTestBase.rewrite_ip_suffix(
                subnet_mask,
                self.server_ip,
                INTENDED_IP_SUFFIX)
        # Pick an address that's unlikely to be in the broadcast domain of the
        # virtual network pair.
        gateway_ip = "10.11.12.13"
        # Two real name servers, and a bogus one to be unpredictable.
        dns_servers = ['8.8.8.8', '8.8.4.4', '192.168.87.88']
        vendor_options = 'ANDROID_METERED'
        # This is the pool of information the server will give out to the client
        # upon request.
        dhcp_options = {
                dhcp_packet.OPTION_SERVER_ID : self.server_ip,
                dhcp_packet.OPTION_SUBNET_MASK : subnet_mask,
                dhcp_packet.OPTION_IP_LEASE_TIME : LEASE_TIME_SECONDS,
                dhcp_packet.OPTION_REQUESTED_IP : intended_ip,
                dhcp_packet.OPTION_DNS_SERVERS : dns_servers,
                dhcp_packet.OPTION_ROUTERS : [ gateway_ip ],
                }
        rules = [
                dhcp_handling_rule.DhcpHandlingRule_RespondToDiscovery(
                        intended_ip, self.server_ip, dhcp_options, {}),
                dhcp_handling_rule.DhcpHandlingRule_RespondToRequest(
                        intended_ip, self.server_ip, dhcp_options, {})
                ]
        rules[-1].is_final_handler = True

        self.server.start_test(rules, DHCP_NEGOTIATION_TIMEOUT_SECONDS)
        self.server.wait_for_test_to_finish()
        if not self.server.last_test_passed:
            raise error.TestFail('Test server didn\'t get all the messages it '
                                 'was told to expect during negotiation.')

        self.wait_for_dhcp_propagation()
        interface_name = self.ethernet_pair.peer_interface_name
        self.check_shill_gateway_setup(interface_name, gateway_ip)
        self.check_routing_table_gateway_setup(interface_name, gateway_ip)
