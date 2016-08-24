# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import dhcp_handling_rule
from autotest_lib.client.cros import dhcp_packet
from autotest_lib.client.cros import dhcp_test_base
from autotest_lib.client.cros import shill_temporary_profile

class network_DhcpStaticIP(dhcp_test_base.DhcpTestBase):
    """DHCP test which confirms static IP functionality"""
    # Length of time the lease from the DHCP server is valid.
    LEASE_TIME_SECONDS = 60
    # We'll fill in the subnet and give this address to the client over DHCP.
    INTENDED_IP_SUFFIX = '0.0.0.101'
    # We'll fill in the subnet and supply this as a static IP address.
    STATIC_IP_SUFFIX = '0.0.0.201'
    STATIC_IP_NAME_SERVERS = [ '1.1.2.2', '1.1.3.4' ]
    # Time to wait for the DHCP negotiation protocol to complete.
    DHCP_NEGOTIATION_TIMEOUT_SECONDS = 10
    # Time to wait after DHCP negotiation completes until service is marked
    # as connected.
    DHCP_SETUP_TIMEOUT_SECONDS = 3
    # Name given to the temporary shill profile we create for this test.
    TEST_PROFILE_NAME = 'TestStaticIP'
    # Various parameters that can be set statically.
    CONFIGURE_STATIC_IP_ADDRESS = 'ip-address'
    CONFIGURE_STATIC_IP_DNS_SERVERS = 'dns-servers'

    def configure_static_ip(self, service, params):
        """Configures the Static IP parameters for the Ethernet interface
        |interface_name| and applies those parameters to the interface by
        forcing a re-connect.

        @param service object the Service DBus interface to configure.
        @param params list of static parameters to set on the service.

        """

        self._static_ip_options = {}
        if self.CONFIGURE_STATIC_IP_ADDRESS in params:
            subnet_mask = self.ethernet_pair.interface_subnet_mask
            static_ip_address = dhcp_test_base.DhcpTestBase.rewrite_ip_suffix(
                    subnet_mask,
                    self.server_ip,
                    self.STATIC_IP_SUFFIX)
            prefix_len = self.ethernet_pair.interface_prefix
            service.SetProperty('StaticIP.Address', static_ip_address)
            service.SetProperty('StaticIP.Prefixlen', prefix_len)
            self._static_ip_options[dhcp_packet.OPTION_REQUESTED_IP] = (
                    static_ip_address)
        if self.CONFIGURE_STATIC_IP_DNS_SERVERS in params:
            service.SetProperty('StaticIP.NameServers',
                                ','.join(self.STATIC_IP_NAME_SERVERS))
            self._static_ip_options[dhcp_packet.OPTION_DNS_SERVERS] = (
                    self.STATIC_IP_NAME_SERVERS)
        service.Disconnect()
        service.Connect()


    def clear_static_ip(self, service, params):
        """Clears configuration of Static IP parameters for the Ethernet
        interface and forces a re-connect.

        @param service object the Service DBus interface to clear properties.
        @param params list of static parameters to clear from the service.

        """
        if self.CONFIGURE_STATIC_IP_ADDRESS in params:
            service.ClearProperty('StaticIP.Address')
            service.ClearProperty('StaticIP.Prefixlen')
        if self.CONFIGURE_STATIC_IP_DNS_SERVERS in params:
            service.ClearProperty('StaticIP.NameServers')
        service.Disconnect()
        service.Connect()


    def check_saved_ip(self, service, options):
        """Check the properties of the Ethernet service to make sure that
        the address provided by the DHCP server is properly added to the
        "Saved.Address".

        @param service object the Service DBus interface to clear properties.
        @param options dict parameters that were used to configure the DHCP
            server.

        """
        intended_ip = options[dhcp_packet.OPTION_REQUESTED_IP]
        properties = service.GetProperties()
        if intended_ip != properties['SavedIP.Address']:
            raise error.TestFail('Saved IP address %s is not DHCP address %s' %
                                 (properties['SavedIP.Address'], intended_ip))


    def make_lease_negotiation_rules(self, options):
        """Generate a set of lease negotiation handling rules for a
        server that will successfully return an IP address to the client.

        @param options dict of options to be negotiated.  In particular,
            the dhcp_packet.OPTION_REQUESTED_IP element is used to configure
            the address that will be returned to the client.
        @return array of DhcpHandlingRule instances which implement the
            negotiation.

        """
        intended_ip = options[dhcp_packet.OPTION_REQUESTED_IP]
        rules = []
        rules.append(dhcp_handling_rule.DhcpHandlingRule_RespondToDiscovery(
                intended_ip,
                self.server_ip,
                options,
                {}))
        rules.append(dhcp_handling_rule.DhcpHandlingRule_RespondToRequest(
                intended_ip,
                self.server_ip,
                options,
                {}))
        return rules


    def test_dhcp_negotiation(self, rules, service):
        """Perform a DHCP lease negotiation using handler rules from |rules|,
        and ensure that |service| becomes connected as a result.

        @param rules array of handling rules that must complete in order for
            the negotiation to be considered successful.
        @param service Service DBus object which should become connected as
            a result of the DHCP negotiation.

        """
        rules[-1].is_final_handler = True
        self.server.start_test(rules, self.DHCP_NEGOTIATION_TIMEOUT_SECONDS)
        self.server.wait_for_test_to_finish()
        if not self.server.last_test_passed:
            raise error.TestFail('Test server didn\'t get all the messages it '
                                 'was told to expect during negotiation.')
        # Wait for the service to enter a "good" state.
        connect_result = self.shill_proxy.wait_for_property_in(
                service,
                self.shill_proxy.SERVICE_PROPERTY_STATE,
                ('ready', 'portal', 'online'),
                self.DHCP_SETUP_TIMEOUT_SECONDS)
        (successful, _, association_time) = connect_result
        if not successful:
            raise error.TestFail('Ethernet service did not become connected.')


    def connect_dynamic_ip(self, options, service):
        """Perform a DHCP negotiation, using |options|.  Then check that
           the IP information configured on client matches the parameters
           in |options|.

        @param options dict containing DHCP packet options to be returned
            to the client during negotiation, and then later checked for
            consistency.
        @param service DBus object of the service that should become
            connected as a result of the negotiation.

        """
        self.test_dhcp_negotiation(self.make_lease_negotiation_rules(options),
                                   service)
        self.check_dhcp_config(options)


    def connect_static_ip(self, options, service, params):
        """Perform a DHCP negotiation, using |options|.  Then check that
           the IP information configured on client matches the parameters
           in |options|, except that the client's IP address should be
           |static_ip_address|.

        @param options dict containing DHCP packet options to be returned
            to the client during negotiation, and then later checked for
            consistency.
        @param service DBus object of the service that should become
            connected as a result of the negotiation.
        @param params list of static IP parameters we will be verifying.

        """
        rules = self.make_lease_negotiation_rules(options)
        if self.CONFIGURE_STATIC_IP_ADDRESS in params:
            # Add a rule that expects the client to release the lease.
            rules.append(dhcp_handling_rule.DhcpHandlingRule_AcceptRelease(
                    self.server_ip,
                    options,
                    {}))
        self.test_dhcp_negotiation(rules, service)

        # Check to make sure that the configured IP address of the client
        # matches the configured static IP address.
        static_ip_options = options.copy()
        static_ip_options.update(self._static_ip_options)
        self.check_dhcp_config(static_ip_options)
        self.check_saved_ip(service, options)


    def test_body(self):
        """The test main body"""
        subnet_mask = self.ethernet_pair.interface_subnet_mask
        intended_ip = dhcp_test_base.DhcpTestBase.rewrite_ip_suffix(
                subnet_mask,
                self.server_ip,
                self.INTENDED_IP_SUFFIX)
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
                dhcp_packet.OPTION_IP_LEASE_TIME : self.LEASE_TIME_SECONDS,
                dhcp_packet.OPTION_REQUESTED_IP : intended_ip,
                dhcp_packet.OPTION_DNS_SERVERS : dns_servers,
                dhcp_packet.OPTION_DOMAIN_NAME : domain_name,
                dhcp_packet.OPTION_DNS_DOMAIN_SEARCH_LIST : dns_search_list,
                }
        service = self.find_ethernet_service(
                self.ethernet_pair.peer_interface_name)

        manager = self.shill_proxy.manager
        with shill_temporary_profile.ShillTemporaryProfile(
                manager, profile_name=self.TEST_PROFILE_NAME):

            self.connect_dynamic_ip(dhcp_options, service)

            for params in self._static_param_list:
                self.configure_static_ip(service, params)
                self.connect_static_ip(dhcp_options, service, params)
                self.clear_static_ip(service, params)

            self.connect_dynamic_ip(dhcp_options, service)


    def run_once(self, static_param_list):
        """Setup the static parameter list before calling the DhcpTestBase
        main loop.

        @param static_param_list list of iterable properties to configure
            for each static IP test.

        """
        self._static_param_list = static_param_list
        super(network_DhcpStaticIP, self).run_once()
