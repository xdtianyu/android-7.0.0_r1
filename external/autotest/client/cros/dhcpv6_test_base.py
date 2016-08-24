# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
Base class for DHCPv6 tests.  This class just sets up a little bit of plumbing,
like a virtual ethernet device with one end that looks like a real ethernet
device to shill and a DHCPv6 test server on the end that doesn't look like a
real ethernet interface to shill.  Child classes should override test_body()
with the logic of their test.
"""

import logging
import time
import traceback

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import virtual_ethernet_pair
from autotest_lib.client.cros import dhcpv6_test_server
from autotest_lib.client.cros.networking import shill_proxy

# These are keys that may be used with the DBus dictionary returned from
# Dhcpv6TestBase.get_interface_ipconfig().
DHCPV6_KEY_ADDRESS = 'Address'
DHCPV6_KEY_DELEGATED_PREFIX = 'DelegatedPrefix'
DHCPV6_KEY_DELEGATED_PREFIX_LENGTH = 'DelegatedPrefixLength'
DHCPV6_KEY_NAMESERVERS = 'NameServers'
DHCPV6_KEY_SEARCH_DOMAIN_LIST = 'SearchDomains'

# After DHCPv6 completes, an ipconfig should appear shortly after
IPCONFIG_POLL_COUNT = 5
IPCONFIG_POLL_PERIOD_SECONDS = 1

class Dhcpv6TestBase(test.test):
    """Parent class for tests that work verify DHCPv6 behavior."""
    version = 1

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
        via DHCPv6.  Returns None if no such interface or setting bundle on
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
          if ipconfig_properties['Method'] != 'dhcp6':
              logging.info('Found ipconfig object with method != dhcp6')
              continue
          if dhcp_properties != None:
              raise error.TestFail('Found multiple ipconfig objects '
                                   'with method == dhcp6')
          dhcp_properties = ipconfig_properties
        if dhcp_properties is None:
            logging.info('Did not find IPConfig object with method == dhcp6')
            return None
        logging.info('Got raw dhcp config dbus object: %s.', dhcp_properties)
        return shill_proxy.ShillProxy.dbus2primitive(dhcp_properties)


    def run_once(self):
        self._server = None
        self._server_ip = None
        self._ethernet_pair = None
        self._shill_proxy = shill_proxy.ShillProxy()
        try:
            # TODO(zqiu): enable DHCPv6 for peer interface, either by restarting
            # shill with appropriate command line options or via a new DBUS
            # command.
            self._ethernet_pair = virtual_ethernet_pair.VirtualEthernetPair(
                    interface_ip=None,
                    peer_interface_name='pseudoethernet0',
                    peer_interface_ip=None,
                    interface_ipv6=dhcpv6_test_server.DHCPV6_SERVER_ADDRESS)
            self._ethernet_pair.setup()
            if not self._ethernet_pair.is_healthy:
                raise error.TestFail('Could not create virtual ethernet pair.')
            self._server_ip = self._ethernet_pair.interface_ip
            self._server = dhcpv6_test_server.Dhcpv6TestServer(
                    self._ethernet_pair.interface_name)
            self._server.start()
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
        Return the IP address of the side of the interface that the DHCPv6 test
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


    def check_dhcpv6_config(self):
        """
        Compare the DHCPv6 ipconfig with DHCP lease parameters to ensure
        that the DUT attained the correct values.

        """
        # Retrieve DHCPv6 configuration.
        for attempt in range(IPCONFIG_POLL_COUNT):
            dhcpv6_config = self.get_interface_ipconfig(
                    self.ethernet_pair.peer_interface_name)
            # Wait until both IP address and delegated prefix are obtained.
            if (dhcpv6_config is not None and
                dhcpv6_config.get(DHCPV6_KEY_ADDRESS) and
                dhcpv6_config.get(DHCPV6_KEY_DELEGATED_PREFIX)):
                break;
            time.sleep(IPCONFIG_POLL_PERIOD_SECONDS)
        else:
            raise error.TestFail('Failed to retrieve DHCPv6 ipconfig object '
                                 'from shill.')

        # Verify Non-temporary Address prefix.
        address = dhcpv6_config.get(DHCPV6_KEY_ADDRESS)
        actual_prefix = address[:address.index('::')]
        expected_prefix = dhcpv6_test_server.DHCPV6_SERVER_SUBNET_PREFIX[:
                dhcpv6_test_server.DHCPV6_SERVER_SUBNET_PREFIX.index('::')]
        if actual_prefix != expected_prefix:
            raise error.TestFail('Address prefix mismatch: '
                                 'actual %s expected %s.' %
                                 (actual_prefix, expected_prefix))
        # Verify Non-temporary Address suffix.
        actual_suffix = int(address[address.index('::')+2:], 16)
        if (actual_suffix < dhcpv6_test_server.DHCPV6_ADDRESS_RANGE_LOW or
            actual_suffix > dhcpv6_test_server.DHCPV6_ADDRESS_RANGE_HIGH):
            raise error.TestFail('Invalid address suffix: '
                                 'actual %x expected (%x-%x)' %
                                 (actual_suffix,
                                  dhcpv6_test_server.DHCPV6_ADDRESS_RANGE_LOW,
                                  dhcpv6_test_server.DHCPV6_ADDRESS_RANGE_HIGH))

        # Verify delegated prefix.
        delegated_prefix = dhcpv6_config.get(DHCPV6_KEY_DELEGATED_PREFIX)
        for x in range(
                dhcpv6_test_server.DHCPV6_PREFIX_DELEGATION_INDEX_LOW,
                dhcpv6_test_server.DHCPV6_PREFIX_DELEGATION_INDEX_HIGH+1):
            valid_prefix = \
                    dhcpv6_test_server.DHCPV6_PREFIX_DELEGATION_RANGE_FORMAT % x
            if delegated_prefix == valid_prefix:
                break;
        else:
            raise error.TestFail('Invalid delegated prefix: %s' %
                                 (delegated_prefix))
        # Verify delegated prefix length.
        delegated_prefix_length = \
                int(dhcpv6_config.get(DHCPV6_KEY_DELEGATED_PREFIX_LENGTH))
        expected_delegated_prefix_length = \
                dhcpv6_test_server.DHCPV6_PREFIX_DELEGATION_PREFIX_LENGTH
        if delegated_prefix_length != expected_delegated_prefix_length:
            raise error.TestFail('Delegated prefix length mismatch: '
                                 'actual %d expected %d' %
                                 (delegated_prefix_length,
                                  expected_delegated_prefix_length))

        # Verify name servers.
        actual_name_servers = dhcpv6_config.get(DHCPV6_KEY_NAMESERVERS)
        expected_name_servers = \
                dhcpv6_test_server.DHCPV6_NAME_SERVERS.split(',')
        if actual_name_servers != expected_name_servers:
            raise error.TestFail('Name servers mismatch: actual %r expected %r'
                                 % (actual_name_servers, expected_name_servers))
        # Verify domain search.
        actual_domain_search = dhcpv6_config.get(DHCPV6_KEY_SEARCH_DOMAIN_LIST)
        expected_domain_search = \
                dhcpv6_test_server.DHCPV6_DOMAIN_SEARCH.split(',')
        if actual_domain_search != expected_domain_search:
            raise error.TestFail('Domain search list mismatch: '
                                 'actual %r expected %r' %
                                 (actual_domain_search, expected_domain_search))
