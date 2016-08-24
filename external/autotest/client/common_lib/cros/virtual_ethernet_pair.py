# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""
VirtualEthernetPair provides methods for setting up and tearing down a virtual
ethernet interface for use in tests.  You will probably need to be root on test
devices to use this class.  The constructor allows you to specify your IP's to
assign to both ends of the pair, however, if you wish to leave the interface
unconfigured, simply pass None.  You may also specify the subnet of your ip
addresses.  Failing to do so leaves them with default in ifconfig.

Example usage:
vif = virtual_ethernet_pair.VirtualEthernetPair(interface_name="master",
                                                peer_interface_name="peer",
                                                interface_ip="10.9.8.1/24",
                                                peer_interface_ip=None)
vif.setup()
if not vif.is_healthy:
    # bad things happened while creating the interface
    # ... abort gracefully

interface_name = vif.interface_name
peer_interface_name = vif.peer_interface_name
#... do things with your interface

# You must call this if you want to leave the system in a good state.
vif.teardown()

Alternatively:

with virtual_ethernet_pair.VirtualEthernetPair(...) as vif:
    if not vif.is_healthy:
        # bad things happened while creating the interface
        # ... abort gracefully

    interface_name = vif.interface_name
    peer_interface_name = vif.peer_interface_name
    #... do things with your interface

"""

import logging

from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib.cros.network import interface

class VirtualEthernetPair(object):
    """ Class for configuring virtual ethernet device pair. """

    def __init__(self,
                 interface_name='veth_master',
                 peer_interface_name='veth_slave',
                 interface_ip='10.9.8.1/24',
                 peer_interface_ip='10.9.8.2/24',
                 interface_ipv6=None,
                 peer_interface_ipv6=None,
                 ignore_shutdown_errors=False,
                 host=None):
        """
        Construct a object managing a virtual ethernet pair.  One end of the
        interface will be called |interface_name|, and the peer end
        |peer_interface_name|.  You may get the interface names later with
        VirtualEthernetPair.get_[peer_]interface_name().  The ends of the
        interface are manually configured with the given IPv4 address strings
        (like "10.9.8.2/24").  You may skip the IP configuration by passing None
        as the address for either interface.
        """
        super(VirtualEthernetPair, self).__init__()
        self._is_healthy = True
        self._interface_name = interface_name
        self._peer_interface_name = peer_interface_name
        self._interface_ip = interface_ip
        self._peer_interface_ip = peer_interface_ip
        self._interface_ipv6 = interface_ipv6
        self._peer_interface_ipv6 = peer_interface_ipv6
        self._ignore_shutdown_errors = ignore_shutdown_errors
        self._run = utils.run
        self._host = host
        if host is not None:
            self._run = host.run


    def setup(self):
        """
        Installs a virtual ethernet interface and configures one side with an IP
        address.  First does some sanity checking and tries to remove an
        existing interface by the same name, and logs messages on failures.
        """
        self._is_healthy = False
        if self._either_interface_exists():
            logging.warning('At least one test interface already existed.'
                            '  Attempting to remove.')
            self._remove_test_interface()
            if self._either_interface_exists():
                logging.error('Failed to remove unexpected test '
                              'interface.  Aborting.')
                return

        self._create_test_interface()
        if not self._interface_exists(self._interface_name):
            logging.error('Failed to create master test interface.')
            return

        if not self._interface_exists(self._peer_interface_name):
            logging.error('Failed to create peer test interface.')
            return
        # Unless you tell the firewall about the interface, you're not going to
        # get any IP traffic through.  Since this is basically a loopback
        # device, just allow all traffic.
        for name in (self._interface_name, self._peer_interface_name):
            status = self._run('iptables -I INPUT -i %s -j ACCEPT' % name,
                               ignore_status=True)
            if status.exit_status != 0:
                logging.error('iptables rule addition failed for interface %s',
                              name)
        self._is_healthy = True


    def teardown(self):
        """
        Removes the interface installed by VirtualEthernetPair.setup(), with
        some simple sanity checks that print warnings when either the interface
        isn't there or fails to be removed.
        """
        for name in (self._interface_name, self._peer_interface_name):
            self._run('iptables -D INPUT -i %s -j ACCEPT' % name,
                      ignore_status=True)
        if not self._either_interface_exists():
            logging.warning('VirtualEthernetPair.teardown() called, '
                            'but no interface was found.')
            return

        self._remove_test_interface()
        if self._either_interface_exists():
            logging.error('Failed to destroy test interface.')


    @property
    def is_healthy(self):
        """@return True if virtual ethernet pair is configured."""
        return self._is_healthy


    @property
    def interface_name(self):
        """@return string name of the interface."""
        return self._interface_name


    @property
    def peer_interface_name(self):
        """@return string name of the peer interface."""
        return self._peer_interface_name


    @property
    def interface_ip(self):
        """@return string IPv4 address of the interface."""
        return interface.Interface(self.interface_name).ipv4_address


    @property
    def peer_interface_ip(self):
        """@return string IPv4 address of the peer interface."""
        return interface.Interface(self.peer_interface_name).ipv4_address


    @property
    def interface_subnet_mask(self):
        """@return string IPv4 subnet mask of the interface."""
        return interface.Interface(self.interface_name).ipv4_subnet_mask


    @property
    def interface_prefix(self):
        """@return int IPv4 prefix length."""
        return interface.Interface(self.interface_name).ipv4_prefix


    @property
    def peer_interface_subnet_mask(self):
        """@return string IPv4 subnet mask of the peer interface."""
        return interface.Interface(self.peer_interface_name).ipv4_subnet_mask


    @property
    def interface_mac(self):
        """@return string MAC address of the interface."""
        return interface.Interface(self.interface_name).mac_address


    @property
    def peer_interface_mac(self):
        """@return string MAC address of the peer interface."""
        return interface.Interface(self._peer_interface_name).mac_address


    def __enter__(self):
        self.setup()
        return self


    def __exit__(self, exc_type, exc_value, traceback):
        self.teardown()


    def _interface_exists(self, interface_name):
        """
        Returns True iff we found an interface with name |interface_name|.
        """
        return interface.Interface(interface_name, host=self._host).exists


    def _either_interface_exists(self):
        return (self._interface_exists(self._interface_name) or
                self._interface_exists(self._peer_interface_name))


    def _remove_test_interface(self):
        """
        Remove the virtual ethernet device installed by
        _create_test_interface().
        """
        self._run('ifconfig %s down' % self._interface_name,
                  ignore_status=self._ignore_shutdown_errors)
        self._run('ifconfig %s down' % self._peer_interface_name,
                  ignore_status=self._ignore_shutdown_errors)
        self._run('ip link delete %s >/dev/null 2>&1' % self._interface_name,
                  ignore_status=self._ignore_shutdown_errors)

        # Under most normal circumstances a successful deletion of
        # |_interface_name| should also remove |_peer_interface_name|,
        # but if we elected to ignore failures above, that may not be
        # the case.
        self._run('ip link delete %s >/dev/null 2>&1' %
                  self._peer_interface_name, ignore_status=True)


    def _create_test_interface(self):
        """
        Set up a virtual ethernet device and configure the host side with a
        fake IP address.
        """
        self._run('ip link add name %s '
                  'type veth peer name %s >/dev/null 2>&1' %
                  (self._interface_name, self._peer_interface_name))
        self._run('ip link set %s up' % self._interface_name)
        self._run('ip link set %s up' % self._peer_interface_name)
        if self._interface_ip is not None:
            self._run('ifconfig %s %s' % (self._interface_name,
                                          self._interface_ip))
        if self._peer_interface_ip is not None:
            self._run('ifconfig %s %s' % (self._peer_interface_name,
                                          self._peer_interface_ip))
        if self._interface_ipv6 is not None:
            self._run('ip -6 addr add %s dev %s' % (self._interface_ipv6,
                                                    self._interface_name))
        if self._peer_interface_ipv6 is not None:
            self._run('ip -6 addr add %s dev %s' % (self._peer_interface_ipv6,
                                                    self._peer_interface_name))