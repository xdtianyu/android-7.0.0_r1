# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
Base class for DHCP tests.  This class just sets up a little bit of plumbing,
like a virtual ethernet device with one end that looks like a real ethernet
device to shill and a DHCP test server on the end that doesn't look like a real
ethernet interface to shill.  Child classes should override test_body() with the
logic of their test.  The plumbing of DhcpTestBase is accessible via properties.
"""

import logging
import socket
import struct
import time
import traceback

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import virtual_ethernet_pair
from autotest_lib.client.cros import dhcp_handling_rule
from autotest_lib.client.cros import dhcp_packet
from autotest_lib.client.cros import dhcp_test_server
from autotest_lib.client.cros.networking import shill_proxy


# These are keys that may be used with the DBus dictionary returned from
# DhcpTestBase.get_interface_ipconfig().
DHCPCD_KEY_NAMESERVERS = 'NameServers'
DHCPCD_KEY_GATEWAY = 'Gateway'
DHCPCD_KEY_BROADCAST_ADDR = 'Broadcast'
DHCPCD_KEY_ADDRESS = 'Address'
DHCPCD_KEY_PREFIX_LENGTH = 'Prefixlen'
DHCPCD_KEY_DOMAIN_NAME = 'DomainName'
DHCPCD_KEY_ACCEPTED_HOSTNAME = 'AcceptedHostname'
DHCPCD_KEY_SEARCH_DOMAIN_LIST = 'SearchDomains'

# We should be able to complete a DHCP negotiation in this amount of time.
DHCP_NEGOTIATION_TIMEOUT_SECONDS = 10

# After DHCP completes, an ipconfig should appear shortly after
IPCONFIG_POLL_COUNT = 5
IPCONFIG_POLL_PERIOD_SECONDS = 0.5

class DhcpTestBase(test.test):
    """Parent class for tests that work verify DHCP behavior."""
    version = 1

    @staticmethod
    def rewrite_ip_suffix(subnet_mask, ip_in_subnet, ip_suffix):
        """
        Create a new IPv4 address in a subnet by bitwise and'ing an existing
        address |ip_in_subnet| with |subnet_mask| and bitwise or'ing in
        |ip_suffix|.  For safety, bitwise or the suffix with the complement of
        the subnet mask.

        Usage: rewrite_ip_suffix("255.255.255.0", "192.168.1.1", "0.0.0.105")

        The example usage will return "192.168.1.105".

        @param subnet_mask string subnet mask, e.g. "255.255.255.0"
        @param ip_in_subnet string an IP address in the desired subnet
        @param ip_suffix string suffix desired for new address, e.g. "0.0.0.105"

        @return string IP address on in the same subnet with specified suffix.

        """
        mask = struct.unpack('!I', socket.inet_aton(subnet_mask))[0]
        subnet = mask & struct.unpack('!I', socket.inet_aton(ip_in_subnet))[0]
        suffix = ~mask & struct.unpack('!I', socket.inet_aton(ip_suffix))[0]
        return socket.inet_ntoa(struct.pack('!I', (subnet | suffix)))


    def get_device(self, interface_name):
        """Finds the corresponding Device object for an interface with
        the name |interface_name|.

        @param interface_name string The name of the interface to check.

        @return DBus interface object representing the associated device.

        """
        return self.shill_proxy.find_object('Device',
                                            {'Name': interface_name})


    def find_ethernet_service(self, interface_name):
        """Finds the corresponding service object for an Ethernet interface.

        @param interface_name string The name of the associated interface

        @return Service object representing the associated service.

        """
        device = self.get_device(interface_name)
        device_path = shill_proxy.ShillProxy.dbus2primitive(device.object_path)
        return self.shill_proxy.find_object('Service', {'Device': device_path})


    def get_interface_ipconfig_objects(self, interface_name):
        """
        Returns a list of dbus object proxies for |interface_name|.
        Returns an empty list if no such interface exists.

        @param interface_name string name of the device to query (e.g., "eth0").

        @return list of objects representing DBus IPConfig RPC endpoints.

        """
        device = self.get_device(interface_name)
        if device is None:
            return []

        device_properties = device.GetProperties(utf8_strings=True)
        proxy = self.shill_proxy

        ipconfig_object = proxy.DBUS_TYPE_IPCONFIG
        return filter(bool,
                      [ proxy.get_dbus_object(ipconfig_object, property_path)
                        for property_path in device_properties['IPConfigs'] ])


    def get_interface_ipconfig(self, interface_name):
        """
        Returns a dictionary containing settings for an |interface_name| set
        via DHCP.  Returns None if no such interface or setting bundle on
        that interface can be found in shill.

        @param interface_name string name of the device to query (e.g., "eth0").

        @return dict containing the the properties of the IPConfig stripped
            of DBus meta-data or None.

        """
        dhcp_properties = None
        for ipconfig in self.get_interface_ipconfig_objects(interface_name):
          logging.info('Looking at ipconfig %r', ipconfig)
          ipconfig_properties = ipconfig.GetProperties(utf8_strings=True)
          if 'Method' not in ipconfig_properties:
              logging.info('Found ipconfig object with no method field')
              continue
          if ipconfig_properties['Method'] != 'dhcp':
              logging.info('Found ipconfig object with method != dhcp')
              continue
          if dhcp_properties != None:
              raise error.TestFail('Found multiple ipconfig objects '
                                   'with method == dhcp')
          dhcp_properties = ipconfig_properties
        if dhcp_properties is None:
            logging.info('Did not find IPConfig object with method == dhcp')
            return None
        logging.info('Got raw dhcp config dbus object: %s.', dhcp_properties)
        return shill_proxy.ShillProxy.dbus2primitive(dhcp_properties)


    def run_once(self):
        self._server = None
        self._server_ip = None
        self._ethernet_pair = None
        self._server = None
        self._shill_proxy = shill_proxy.ShillProxy()
        try:
            self._ethernet_pair = virtual_ethernet_pair.VirtualEthernetPair(
                    peer_interface_name='pseudoethernet0',
                    peer_interface_ip=None)
            self._ethernet_pair.setup()
            if not self._ethernet_pair.is_healthy:
                raise error.TestFail('Could not create virtual ethernet pair.')
            self._server_ip = self._ethernet_pair.interface_ip
            self._server = dhcp_test_server.DhcpTestServer(
                    self._ethernet_pair.interface_name)
            self._server.start()
            if not self._server.is_healthy:
                raise error.TestFail('Could not start DHCP test server.')
            self._subnet_mask = self._ethernet_pair.interface_subnet_mask
            self.test_body()
        except (error.TestFail, error.TestNAError):
            # Pass these through without modification.
            raise
        except Exception as e:
            logging.error('Caught exception: %s.', str(e))
            logging.error('Trace: %s', traceback.format_exc())
            raise error.TestFail('Caught exception: %s.' % str(e))
        finally:
            if self._server is not None:
                self._server.stop()
            if self._ethernet_pair is not None:
                self._ethernet_pair.teardown()

    def test_body(self):
        """
        Override this method with the body of your test.  You may safely assume
        that the the properties exposed by DhcpTestBase correctly return
        references to the test apparatus.
        """
        raise error.TestFail('No test body implemented')

    @property
    def server_ip(self):
        """
        Return the IP address of the side of the interface that the DHCP test
        server is bound to.  The server itself is bound the the broadcast
        address on the interface.
        """
        return self._server_ip

    @property
    def server(self):
        """
        Returns a reference to the DHCP test server.  Use this to add handlers
        and run tests.
        """
        return self._server

    @property
    def ethernet_pair(self):
        """
        Returns a reference to the virtual ethernet pair created to run DHCP
        tests on.
        """
        return self._ethernet_pair

    @property
    def shill_proxy(self):
        """
        Returns a the shill proxy instance.
        """
        return self._shill_proxy

    def negotiate_and_check_lease(self,
                                  dhcp_options,
                                  custom_fields={},
                                  disable_check=False):
        """
        Perform DHCP lease negotiation, and ensure that the resulting
        ipconfig matches the DHCP options provided to the server.

        @param dhcp_options dict of properties the DHCP server should provide.
        @param custom_fields dict of custom DHCP parameters to add to server.
        @param disable_check bool whether to perform IPConfig parameter
             checking.

        """
        if dhcp_packet.OPTION_REQUESTED_IP not in dhcp_options:
            raise error.TestFail('You must specify OPTION_REQUESTED_IP to '
                                 'negotiate a DHCP lease')
        intended_ip = dhcp_options[dhcp_packet.OPTION_REQUESTED_IP]
        # Build up the handling rules for the server and start the test.
        rules = []
        rules.append(dhcp_handling_rule.DhcpHandlingRule_RespondToDiscovery(
                intended_ip,
                self.server_ip,
                dhcp_options,
                custom_fields))
        rules.append(dhcp_handling_rule.DhcpHandlingRule_RespondToRequest(
                intended_ip,
                self.server_ip,
                dhcp_options,
                custom_fields))
        rules[-1].is_final_handler = True
        self.server.start_test(rules, DHCP_NEGOTIATION_TIMEOUT_SECONDS)
        logging.info('Server is negotiating new lease with options: %s',
                     dhcp_options)
        self.server.wait_for_test_to_finish()
        if not self.server.last_test_passed:
            raise error.TestFail(
                'Test failed: active rule is %s' % self.server.current_rule)

        if disable_check:
            logging.info('Skipping check of negotiated DHCP lease parameters.')
        else:
            self.wait_for_dhcp_propagation()
            self.check_dhcp_config(dhcp_options)

    def wait_for_dhcp_propagation(self):
        """
        Wait for configuration to propagate over dbus to shill.
        TODO(wiley) Make this event based.  This is pretty sloppy.
        """
        time.sleep(0.1)

    def check_dhcp_config(self, dhcp_options):
        """
        Compare the DHCP ipconfig with DHCP lease parameters to ensure
        that the DUT attained the correct values.

        @param dhcp_options dict of properties the DHCP server provided.

        """
        # The config is what the interface was actually configured with, as
        # opposed to dhcp_options, which is what the server expected it be
        # configured with.
        for attempt in range(IPCONFIG_POLL_COUNT):
            dhcp_config = self.get_interface_ipconfig(
                    self.ethernet_pair.peer_interface_name)
            if dhcp_config is not None:
                break
            time.sleep(IPCONFIG_POLL_PERIOD_SECONDS)
        else:
            raise error.TestFail('Failed to retrieve DHCP ipconfig object '
                                 'from shill.')

        logging.debug('Got DHCP config: %s', str(dhcp_config))
        expected_address = dhcp_options.get(dhcp_packet.OPTION_REQUESTED_IP)
        configured_address = dhcp_config.get(DHCPCD_KEY_ADDRESS)
        if expected_address != configured_address:
            raise error.TestFail('Interface configured with IP address not '
                                 'granted by the DHCP server after DHCP '
                                 'negotiation.  Expected %s but got %s.' %
                                 (expected_address, configured_address))

        # While DNS related settings only propagate to the system when the
        # service is marked as the default service, we can still check the
        # IP address on the interface, since that is set immediately.
        interface_address = self.ethernet_pair.peer_interface_ip
        if expected_address != interface_address:
            raise error.TestFail('shill somehow knew about the proper DHCP '
                                 'assigned address: %s, but configured the '
                                 'interface with something completely '
                                 'different: %s.' %
                                 (expected_address, interface_address))

        expected_dns_servers = dhcp_options.get(dhcp_packet.OPTION_DNS_SERVERS)
        configured_dns_servers = dhcp_config.get(DHCPCD_KEY_NAMESERVERS)
        if (expected_dns_servers is not None and
            expected_dns_servers != configured_dns_servers):
            raise error.TestFail('Expected to be configured with DNS server '
                                 'list %s, but was configured with %s '
                                 'instead.' % (expected_dns_servers,
                                               configured_dns_servers))

        expected_domain_name = dhcp_options.get(dhcp_packet.OPTION_DOMAIN_NAME)
        configured_domain_name = dhcp_config.get(DHCPCD_KEY_DOMAIN_NAME)
        if (expected_domain_name is not None and
            expected_domain_name != configured_domain_name):
            raise error.TestFail('Expected to be configured with domain '
                                 'name %s, but got %s instead.' %
                                 (expected_domain_name, configured_domain_name))

        expected_host_name = dhcp_options.get(dhcp_packet.OPTION_HOST_NAME)
        configured_host_name = dhcp_config.get(DHCPCD_KEY_ACCEPTED_HOSTNAME)
        if (expected_host_name is not None and
            expected_host_name != configured_host_name):
            raise error.TestFail('Expected to be configured with host '
                                 'name %s, but got %s instead.' %
                                 (expected_host_name, configured_host_name))

        expected_search_list = dhcp_options.get(
                dhcp_packet.OPTION_DNS_DOMAIN_SEARCH_LIST)
        configured_search_list = dhcp_config.get(DHCPCD_KEY_SEARCH_DOMAIN_LIST)
        if (expected_search_list is not None and
            expected_search_list != configured_search_list):
            raise error.TestFail('Expected to be configured with domain '
                                 'search list %s, but got %s instead.' %
                                 (expected_search_list, configured_search_list))

        expected_routers = dhcp_options.get(dhcp_packet.OPTION_ROUTERS)
        if (not expected_routers and
            dhcp_options.get(dhcp_packet.OPTION_CLASSLESS_STATIC_ROUTES)):
            classless_static_routes = dhcp_options[
                dhcp_packet.OPTION_CLASSLESS_STATIC_ROUTES]
            for prefix, destination, gateway in classless_static_routes:
                if not prefix:
                    logging.info('Using %s as the default gateway', gateway)
                    expected_routers = [ gateway ]
                    break
        configured_router = dhcp_config.get(DHCPCD_KEY_GATEWAY)
        if expected_routers and expected_routers[0] != configured_router:
            raise error.TestFail('Expected to be configured with gateway %s, '
                                 'but got %s instead.' %
                                 (expected_routers[0], configured_router))

        self.server.wait_for_test_to_finish()
        if not self.server.last_test_passed:
            raise error.TestFail('Test server didn\'t get all the messages it '
                                 'was told to expect for renewal.')
