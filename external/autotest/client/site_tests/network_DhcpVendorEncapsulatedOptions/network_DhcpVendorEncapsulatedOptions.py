# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.common_lib import error
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

class network_DhcpVendorEncapsulatedOptions(dhcp_test_base.DhcpTestBase):
    """Test implemenation of Vendor Enacapsulated Options in DHCP response."""

    def check_vendor_encapsulated_options(self, option_string):
        """Check that the ipconfig in the client shows the the vendor options.

        @param option_string string expected value for vendor options.

        """
        proxy = shill_proxy.ShillProxy()
        device = proxy.find_object(
                'Device',
                {'Name': self.ethernet_pair.peer_interface_name})
        if device is None:
            raise error.TestFail('Device was not found.')
        device_properties = device.GetProperties(utf8_strings=True)
        ipconfig_path = device_properties['IPConfigs'][0]
        ipconfig = proxy.get_dbus_object('org.chromium.flimflam.IPConfig',
                                         ipconfig_path)
        ipconfig_properties = ipconfig.GetProperties(utf8_strings=True)
        ipconfig_vendor_encapsulated_options = ''.join(map(chr,
            ipconfig_properties['VendorEncapsulatedOptions']))
        if ipconfig_vendor_encapsulated_options != option_string:
            raise error.TestFail('Shill vendor encapsulated options %s does '
                                 'not match expected %s.' %
                                 (ipconfig_vendor_encapsulated_options,
                                  option_string))

        device_path = shill_proxy.ShillProxy.dbus2primitive(device.object_path)
        service = proxy.find_object('Service', {'Device': device_path})
        tethering = service.GetProperties()['Tethering']
        expected_value = 'Confirmed'
        if tethering != expected_value:
            raise error.TestFail('Service tethering state %s does '
                                 'not match expected %s.' %
                                 (tethering, expected_value))


    def test_body(self):
        """Main body of the test."""
        subnet_mask = self.ethernet_pair.interface_subnet_mask
        intended_ip = dhcp_test_base.DhcpTestBase.rewrite_ip_suffix(
                subnet_mask,
                self.server_ip,
                INTENDED_IP_SUFFIX)
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
                dhcp_packet.OPTION_VENDOR_ENCAPSULATED_OPTIONS : vendor_options
                }
        rules = [
                dhcp_handling_rule.DhcpHandlingRule_RespondToDiscovery(
                        intended_ip, self.server_ip, dhcp_options, {}),
                dhcp_handling_rule.DhcpHandlingRule_RespondToRequest(
                        intended_ip, self.server_ip, dhcp_options, {})
                ]
        rules[-1].is_final_handler = True

        # In some DHCP server implementations, the vendor encapsulated option
        # is provided in the DHCP response without the client requesting it.
        for rule in rules:
            rule.force_reply_options = [
                    dhcp_packet.OPTION_VENDOR_ENCAPSULATED_OPTIONS ]

        self.server.start_test(rules, DHCP_NEGOTIATION_TIMEOUT_SECONDS)
        self.server.wait_for_test_to_finish()
        if not self.server.last_test_passed:
            raise error.TestFail('Test server didn\'t get all the messages it '
                                 'was told to expect during negotiation.')

        self.wait_for_dhcp_propagation()
        self.check_vendor_encapsulated_options(vendor_options)
