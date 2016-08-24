# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import time

from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import dhcp_test_base

class network_DhcpFailureWithStaticIP(dhcp_test_base.DhcpTestBase):
    """The DHCP Negotiation Timeout class.

    Sets up a virtual ethernet pair, and stops the DHCP server on the
    pair.  Static IP parameters are configured on the interface using
    shill's StaticIP configuration.  Ensure that these parameters are
    immediately applied to the ipconfig.

    After the DHCP timeout interval, check to make sure that the same
    IP config remains applied.

    """
    SHILL_DHCP_TIMEOUT_SECONDS = 30


    def check_static_ip_config(self, ipconfig, static_ip_address, name_servers):
        """Checks that the static IP configuration is applied to the
        interface ipconfig.

        @param ipconfig object representing the DBus IPConfig entity to check.
        @param static_ip_address string IP address we expect to be configured.
        @param name_servers list of string name servers we expect to be
                configured on the interface.

        """
        ipconfig_properties = self.shill_proxy.dbus2primitive(
                ipconfig.GetProperties(utf8_strings=True))

        logging.info('IPConfig properties are %r', ipconfig_properties)
        if static_ip_address != ipconfig_properties['Address']:
            raise error.TestFail('Expected address %r but got %r' %
                                 (static_ip_address,
                                  ipconfig_properties['Address']))

        if name_servers != ipconfig_properties['NameServers']:
            raise error.TestFail('Expected name servers %r but got %r' %
                                 (name_servers,
                                  ipconfig_properties['NameServers']))


    def get_ipconfig(self):
        """Returns the first IPConfig object associated with the peer device."""
        ipconfig_objects = (
                self.get_interface_ipconfig_objects(
                        self.ethernet_pair.peer_interface_name))
        if len(ipconfig_objects) == 0:
            raise error.TestFail('Failed to retrieve DHCP ipconfig object '
                                 'from shill.')
        return ipconfig_objects[0]


    def test_body(self):
        """Test main loop."""
        self.server.stop()
        service = self.find_ethernet_service(
                self.ethernet_pair.peer_interface_name)

        static_ip_address = '192.168.1.101'
        prefix_len = 23
        service.SetProperty('StaticIP.Address', static_ip_address)
        service.SetProperty('StaticIP.Prefixlen', prefix_len)
        name_servers = [ '10.10.10.10', '10.10.11.11' ]
        service.SetProperty('StaticIP.NameServers',
                            ','.join(name_servers))
        ipconfig = self.get_ipconfig()
        ipconfig.Refresh()

        self.check_static_ip_config(ipconfig, static_ip_address, name_servers)

        # Make sure configuration is still correct after DHCP timeout.
        time.sleep(self.SHILL_DHCP_TIMEOUT_SECONDS + 2)
        ipconfig = self.get_ipconfig()
        self.check_static_ip_config(ipconfig, static_ip_address, name_servers)
