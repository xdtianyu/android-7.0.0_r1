# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import urlparse


import common
from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import virtual_ethernet_pair
from autotest_lib.client.cros import network, network_chroot
from autotest_lib.client.cros.cellular import test_endpoint

class PseudoNetInterface(object):
    """
    PseudoNetInterface provides a pseudo modem network interface.  This
    network interface is one end of a virtual Ethernet pair.  The other end
    of the virtual Ethernet pair is connected to a minijail that provides DHCP
    and DNS services.  Also in the minijail is a test endpoint (web server)
    that is needed to pass portal detection and perform data transfer tests.

    """
    ARP_ANNOUNCE_CONF = '/proc/sys/net/ipv4/conf/all/arp_announce'
    IFACE_NAME = 'pseudomodem0'
    PEER_IFACE_NAME = IFACE_NAME + 'p'
    IFACE_IP_BASE = '192.168.7'
    IFACE_NETWORK_PREFIX = 24
    NETWORK_CHROOT_CONFIG = {
            'etc/passwd' :
                    'root:x:0:0:root:/root:/bin/bash\n'
                    'nobody:x:65534:65534:nobody:/var/empty:/bin/false\n',
            'etc/group' :
                    'nobody::65534:\n'}
    SHILL_PORTAL_DETECTION_SERVER = 'www.gstatic.com'

    def __init__(self):
        self._arp_announce = 0
        peer_ip = self.IFACE_IP_BASE + '.1'
        peer_interface_ip = peer_ip + '/' + str(self.IFACE_NETWORK_PREFIX)
        self.vif = virtual_ethernet_pair.VirtualEthernetPair(
                interface_name=self.IFACE_NAME,
                peer_interface_name=self.PEER_IFACE_NAME,
                interface_ip=None,
                peer_interface_ip=peer_interface_ip,
                ignore_shutdown_errors=True)
        self.chroot = network_chroot.NetworkChroot(self.PEER_IFACE_NAME,
                                                   peer_ip,
                                                   self.IFACE_NETWORK_PREFIX)
        self.chroot.add_config_templates(self.NETWORK_CHROOT_CONFIG)
        self.chroot.add_startup_command(
                'iptables -I INPUT -p udp --dport 67 -j ACCEPT')
        self.chroot.add_startup_command(
                'iptables -I INPUT -p tcp --dport 80 -j ACCEPT')
        self._dnsmasq_command = self._GetDnsmasqCommand(peer_ip)
        self.chroot.add_startup_command(self._dnsmasq_command)
        self._test_endpoint_command = self._GetTestEndpointCommand()
        self.chroot.add_startup_command(self._test_endpoint_command)

    @staticmethod
    def _GetDnsmasqCommand(peer_ip):
        dnsmasq_command = (
                'dnsmasq '
                '--dhcp-leasefile=/tmp/dnsmasq.leases '
                '--dhcp-range=%s.2,%s.254 '
                '--no-resolv '
                '--no-hosts ' %
                (PseudoNetInterface.IFACE_IP_BASE,
                 PseudoNetInterface.IFACE_IP_BASE))
        test_fetch_url_host = \
                urlparse.urlparse(network.FETCH_URL_PATTERN_FOR_TEST).netloc
        dns_lookup_table = {
                PseudoNetInterface.SHILL_PORTAL_DETECTION_SERVER: peer_ip,
                test_fetch_url_host: peer_ip }
        for host, ip in dns_lookup_table.iteritems():
            dnsmasq_command += '--address=/%s/%s ' % (host, ip)
        return dnsmasq_command

    @staticmethod
    def _GetTestEndpointCommand():
        test_endpoint_path = os.path.abspath(test_endpoint.__file__)
        if test_endpoint_path.endswith('.pyc'):
            test_endpoint_path = test_endpoint_path[:-1]
        return test_endpoint_path

    def _ChrootRunCmdIgnoreErrors(self, cmd):
        try:
            self.chroot.run(cmd)
        except error.CmdError:
            pass

    def BringInterfaceUp(self):
        """
        Brings up the pseudo modem network interface.

        """
        utils.run('sudo ifconfig %s up' % self.IFACE_NAME)

    def BringInterfaceDown(self):
        """
        Brings down the pseudo modem network interface.

        """
        utils.run('sudo ifconfig %s down' % self.IFACE_NAME);

    def Setup(self):
        """
        Sets up the virtual Ethernet pair and starts dnsmasq.

        """
        # Make sure ARP requests for the pseudo modem network addresses
        # go out the pseudo modem network interface.
        self._arp_announce = utils.system_output(
                'cat %s' % self.ARP_ANNOUNCE_CONF)
        utils.run('echo 1 > %s' % self.ARP_ANNOUNCE_CONF)

        self.vif.setup()
        self.BringInterfaceDown()
        if not self.vif.is_healthy:
            raise Exception('Could not initialize virtual ethernet pair')
        self.chroot.startup()

    def Teardown(self):
        """
        Stops dnsmasq and takes down the virtual Ethernet pair.

        """
        self._ChrootRunCmdIgnoreErrors(['/bin/bash', '-c', '"pkill dnsmasq"'])
        self._ChrootRunCmdIgnoreErrors(['/bin/bash', '-c',
                                        '"pkill -f test_endpoint"'])
        self.vif.teardown()
        self.chroot.shutdown()
        utils.run('echo %s > %s' % (self._arp_announce, self.ARP_ANNOUNCE_CONF))

    def Restart(self):
        """
        Restarts the configuration.

        """
        self.Teardown()
        self.Setup()

