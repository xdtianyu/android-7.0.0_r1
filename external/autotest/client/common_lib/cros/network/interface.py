# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import collections
import logging
import os
import re

from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros.network import netblock

# A tuple consisting of a readable part number (one of NAME_* below)
# and a kernel module that provides the driver for this part (e.g. ath9k).
DeviceDescription = collections.namedtuple('DeviceDescription',
                                           ['name', 'kernel_module'])


# A tuple describing a default route, consisting of an interface name,
# gateway IP address, and the metric value visible in the routing table.
DefaultRoute = collections.namedtuple('DefaultRoute', ['interface_name',
                                                       'gateway',
                                                       'metric'])

NAME_MARVELL_88W8797_SDIO = 'Marvell 88W8797 SDIO'
NAME_MARVELL_88W8887_SDIO = 'Marvell 88W8887 SDIO'
NAME_MARVELL_88W8897_SDIO = 'Marvell 88W8897 SDIO'
NAME_MARVELL_88W8897_PCIE = 'Marvell 88W8897 PCIE'
NAME_ATHEROS_AR9280 = 'Atheros AR9280'
NAME_ATHEROS_AR9382 = 'Atheros AR9382'
NAME_ATHEROS_AR9462 = 'Atheros AR9462'
NAME_INTEL_7260 = 'Intel 7260'
NAME_INTEL_7265 = 'Intel 7265'
NAME_BROADCOM_BCM4354_SDIO = 'Broadcom BCM4354 SDIO'
NAME_BROADCOM_BCM4356_PCIE = 'Broadcom BCM4356 PCIE'
NAME_BROADCOM_BCM4371_PCIE = 'Broadcom BCM4371 PCIE'
NAME_UNKNOWN = 'Unknown WiFi Device'

DEVICE_INFO_ROOT = '/sys/class/net'
DeviceInfo = collections.namedtuple('DeviceInfo', ['vendor', 'device'])
DEVICE_NAME_LOOKUP = {
    DeviceInfo('0x02df', '0x9129'): NAME_MARVELL_88W8797_SDIO,
    DeviceInfo('0x02df', '0x912d'): NAME_MARVELL_88W8897_SDIO,
    DeviceInfo('0x02df', '0x9135'): NAME_MARVELL_88W8887_SDIO,
    DeviceInfo('0x11ab', '0x2b38'): NAME_MARVELL_88W8897_PCIE,
    DeviceInfo('0x168c', '0x002a'): NAME_ATHEROS_AR9280,
    DeviceInfo('0x168c', '0x0030'): NAME_ATHEROS_AR9382,
    DeviceInfo('0x168c', '0x0034'): NAME_ATHEROS_AR9462,
    DeviceInfo('0x8086', '0x08b1'): NAME_INTEL_7260,
    # TODO(wiley): Why is this number slightly different on some platforms?
    #              Is it just a different part source?
    DeviceInfo('0x8086', '0x08b2'): NAME_INTEL_7260,
    DeviceInfo('0x8086', '0x095a'): NAME_INTEL_7265,
    DeviceInfo('0x02d0', '0x4354'): NAME_BROADCOM_BCM4354_SDIO,
    DeviceInfo('0x14e4', '0x43ec'): NAME_BROADCOM_BCM4356_PCIE,
    DeviceInfo('0x14e4', '0x440d'): NAME_BROADCOM_BCM4371_PCIE,
}

class Interface:
    """Interace is a class that contains the queriable address properties
    of an network device.
    """
    ADDRESS_TYPE_MAC = 'link/ether'
    ADDRESS_TYPE_IPV4 = 'inet'
    ADDRESS_TYPE_IPV6 = 'inet6'
    ADDRESS_TYPES = [ ADDRESS_TYPE_MAC, ADDRESS_TYPE_IPV4, ADDRESS_TYPE_IPV6 ]

    INTERFACE_NAME_ETHERNET = 'eth0'  # Assume this is `the` ethernet interface.


    @staticmethod
    def get_connected_ethernet_interface(ignore_failures=False):
        """Get an interface object representing a connected ethernet device.

        Raises an exception if no such interface exists.

        @param ignore_failures bool function will return None instead of raising
                an exception on failures.
        @return an Interface object except under the conditions described above.

        """
        # Assume that ethernet devices are called ethX until proven otherwise.
        for device_name in ['eth%d' % i for i in range(5)]:
            ethernet_if = Interface(device_name)
            if ethernet_if.exists and ethernet_if.ipv4_address:
                return ethernet_if

        else:
            if ignore_failures:
                return None

            raise error.TestFail('Failed to find ethernet interface.')


    def __init__(self, name, host=None):
        self._name = name
        self._run = utils.run
        if host is not None:
            self._run = host.run


    @property
    def name(self):
        """@return name of the interface (e.g. 'wlan0')."""
        return self._name


    @property
    def addresses(self):
        """@return the addresses (MAC, IP) associated with interface."""
        # "ip addr show %s 2> /dev/null" returns something that looks like:
        #
        # 2: eth0: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc pfifo_fast
        #    link/ether ac:16:2d:07:51:0f brd ff:ff:ff:ff:ff:ff
        #    inet 172.22.73.124/22 brd 172.22.75.255 scope global eth0
        #    inet6 2620:0:1000:1b02:ae16:2dff:fe07:510f/64 scope global dynamic
        #       valid_lft 2591982sec preferred_lft 604782sec
        #    inet6 fe80::ae16:2dff:fe07:510f/64 scope link
        #       valid_lft forever preferred_lft forever
        #
        # We extract the second column from any entry for which the first
        # column is an address type we are interested in.  For example,
        # for "inet 172.22.73.124/22 ...", we will capture "172.22.73.124/22".
        result = self._run('ip addr show %s 2> /dev/null' % self._name,
                           ignore_status=True)
        address_info = result.stdout
        if result.exit_status != 0:
            # The "ip" command will return non-zero if the interface does
            # not exist.
            return {}

        addresses = {}
        for address_line in address_info.splitlines():
            address_parts = address_line.lstrip().split()
            if len(address_parts) < 2:
                continue
            address_type, address_value = address_parts[:2]
            if address_type in self.ADDRESS_TYPES:
                if address_type not in addresses:
                    addresses[address_type] = []
                addresses[address_type].append(address_value)
        return addresses


    @property
    def device_description(self):
        """@return DeviceDescription object for a WiFi interface, or None."""
        exists = lambda path: self._run(
                'test -e "%s"' % path,
                ignore_status=True).exit_status == 0
        read_file = (lambda path: self._run('cat "%s"' % path).stdout.rstrip()
                     if exists(path) else None)
        if not self.is_wifi_device:
            logging.error('Device description not supported on non-wifi '
                          'interface: %s.', self._name)
            return None

        # This assumes that our path separator is the same as the remote host.
        device_path = os.path.join(DEVICE_INFO_ROOT, self._name, 'device')
        if not exists(device_path):
            logging.error('No device information found at %s', device_path)
            return None

        # TODO(benchan): The 'vendor' / 'device' files do not always exist
        # under the device path. We probably need to figure out an alternative
        # way to determine the vendor and device ID.
        vendor_id = read_file(os.path.join(device_path, 'vendor'))
        product_id = read_file(os.path.join(device_path, 'device'))
        driver_info = DeviceInfo(vendor_id, product_id)
        if driver_info in DEVICE_NAME_LOOKUP:
            device_name = DEVICE_NAME_LOOKUP[driver_info]
            logging.debug('Device is %s',  device_name)
        else:
            logging.error('Device vendor/product pair %r for device %s is '
                          'unknown!', driver_info, product_id)
            device_name = NAME_UNKNOWN
        module_readlink_result = self._run('readlink "%s"' %
                os.path.join(device_path, 'driver', 'module'),
                ignore_status=True)
        if module_readlink_result.exit_status == 0:
            module_name = os.path.basename(
                    module_readlink_result.stdout.strip())
            kernel_release = self._run('uname -r').stdout.strip()
            module_path = self._run('find '
                                    '/lib/modules/%s/kernel/drivers/net '
                                    '-name %s.ko -printf %%P' %
                                    (kernel_release, module_name)).stdout
        else:
            module_path = 'Unknown (kernel might have modules disabled)'
        return DeviceDescription(device_name, module_path)


    @property
    def exists(self):
        """@return True if this interface exists, False otherwise."""
        # No valid interface has no addresses at all.
        return bool(self.addresses)


    @property
    def is_up(self):
        """@return True if this interface is UP, False otherwise."""
        # "ip addr show %s 2> /dev/null" returns something that looks like:
        #
        # 2: eth0: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc pfifo_fast
        #    link/ether ac:16:2d:07:51:0f brd ff:ff:ff:ff:ff:ff
        #    inet 172.22.73.124/22 brd 172.22.75.255 scope global eth0
        #    inet6 2620:0:1000:1b02:ae16:2dff:fe07:510f/64 scope global dynamic
        #       valid_lft 2591982sec preferred_lft 604782sec
        #    inet6 fe80::ae16:2dff:fe07:510f/64 scope link
        #       valid_lft forever preferred_lft forever
        #
        # We only cares about the flags in the first line.
        result = self._run('ip addr show %s 2> /dev/null' % self._name,
                           ignore_status=True)
        address_info = result.stdout
        if result.exit_status != 0:
            # The "ip" command will return non-zero if the interface does
            # not exist.
            return False
        status_line = address_info.splitlines()[0]
        flags_str = status_line[status_line.find('<')+1:status_line.find('>')]
        flags = flags_str.split(',')
        if 'UP' not in flags:
            return False
        return True


    @property
    def mac_address(self):
        """@return the (first) MAC address, e.g., "00:11:22:33:44:55"."""
        return self.addresses.get(self.ADDRESS_TYPE_MAC, [None])[0]


    @property
    def ipv4_address_and_prefix(self):
        """@return the IPv4 address/prefix, e.g., "192.186.0.1/24"."""
        return self.addresses.get(self.ADDRESS_TYPE_IPV4, [None])[0]


    @property
    def ipv4_address(self):
        """@return the (first) IPv4 address, e.g., "192.168.0.1"."""
        netblock_addr = self.netblock
        return netblock_addr.addr if netblock_addr else None


    @property
    def ipv4_prefix(self):
        """@return the IPv4 address prefix e.g., 24."""
        addr = self.netblock
        return addr.prefix_len if addr else None


    @property
    def ipv4_subnet(self):
        """@return string subnet of IPv4 address (e.g. '192.168.0.0')"""
        addr = self.netblock
        return addr.subnet if addr else None


    @property
    def ipv4_subnet_mask(self):
        """@return the IPv4 subnet mask e.g., "255.255.255.0"."""
        addr = self.netblock
        return addr.netmask if addr else None


    def is_wifi_device(self):
        """@return True if iw thinks this is a wifi device."""
        if self._run('iw dev %s info' % self._name,
                     ignore_status=True).exit_status:
            logging.debug('%s does not seem to be a wireless device.',
                          self._name)
            return False
        return True


    @property
    def netblock(self):
        """Return Netblock object for this interface's IPv4 address.

        @return Netblock object (or None if no IPv4 address found).

        """
        netblock_str = self.ipv4_address_and_prefix
        return netblock.from_addr(netblock_str) if netblock_str else None


    @property
    def signal_level(self):
        """Get the signal level for an interface.

        This is currently only defined for WiFi interfaces.

        localhost test # iw dev mlan0 link
        Connected to 04:f0:21:03:7d:b2 (on mlan0)
                SSID: Perf_slvf0_ch36
                freq: 5180
                RX: 699407596 bytes (8165441 packets)
                TX: 58632580 bytes (9923989 packets)
                signal: -54 dBm
                tx bitrate: 130.0 MBit/s MCS 15

                bss flags:
                dtim period:    2
                beacon int:     100

        @return signal level in dBm (a negative, integral number).

        """
        if not self.is_wifi_device():
            return None

        result_lines = self._run('iw dev %s link' %
                                 self._name).stdout.splitlines()
        signal_pattern = re.compile('signal:\s+([-0-9]+)\s+dbm')
        for line in result_lines:
            cleaned = line.strip().lower()
            match = re.search(signal_pattern, cleaned)
            if match is not None:
                return int(match.group(1))

        logging.error('Failed to find signal level for %s.', self._name)
        return None


    @property
    def mtu(self):
        """@return the interface configured maximum transmission unit (MTU)."""
        # "ip addr show %s 2> /dev/null" returns something that looks like:
        #
        # 2: eth0: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc pfifo_fast
        #    link/ether ac:16:2d:07:51:0f brd ff:ff:ff:ff:ff:ff
        #    inet 172.22.73.124/22 brd 172.22.75.255 scope global eth0
        #    inet6 2620:0:1000:1b02:ae16:2dff:fe07:510f/64 scope global dynamic
        #       valid_lft 2591982sec preferred_lft 604782sec
        #    inet6 fe80::ae16:2dff:fe07:510f/64 scope link
        #       valid_lft forever preferred_lft forever
        #
        # We extract the 'mtu' value (in this example "1500")
        try:
            result = self._run('ip addr show %s 2> /dev/null' % self._name)
            address_info = result.stdout
        except error.CmdError, e:
            # The "ip" command will return non-zero if the interface does
            # not exist.
            return None

        match = re.search('mtu\s+(\d+)', address_info)
        if not match:
            raise error.TestFail('MTU information is not available.')
        return int(match.group(1))


    def noise_level(self, frequency_mhz):
        """Get the noise level for an interface at a given frequency.

        This is currently only defined for WiFi interfaces.

        This only works on some devices because 'iw survey dump' (the method
        used to get the noise) only works on some devices.  On other devices,
        this method returns None.

        @param frequency_mhz: frequency at which the noise level should be
               measured and reported.
        @return noise level in dBm (a negative, integral number) or None.

        """
        if not self.is_wifi_device():
            return None

        # This code has to find the frequency and then find the noise
        # associated with that frequency because 'iw survey dump' output looks
        # like this:
        #
        # localhost test # iw dev mlan0 survey dump
        # ...
        # Survey data from mlan0
        #     frequency:              5805 MHz
        #     noise:                  -91 dBm
        #     channel active time:    124 ms
        #     channel busy time:      1 ms
        #     channel receive time:   1 ms
        #     channel transmit time:  0 ms
        # Survey data from mlan0
        #     frequency:              5825 MHz
        # ...

        result_lines = self._run('iw dev %s survey dump' %
                                 self._name).stdout.splitlines()
        my_frequency_pattern = re.compile('frequency:\s*%d mhz' %
                                          frequency_mhz)
        any_frequency_pattern = re.compile('frequency:\s*\d{4} mhz')
        inside_desired_frequency_block = False
        noise_pattern = re.compile('noise:\s*([-0-9]+)\s+dbm')
        for line in result_lines:
            cleaned = line.strip().lower()
            if my_frequency_pattern.match(cleaned):
                inside_desired_frequency_block = True
            elif inside_desired_frequency_block:
                match = noise_pattern.match(cleaned)
                if match is not None:
                    return int(match.group(1))
                if any_frequency_pattern.match(cleaned):
                    inside_desired_frequency_block = False

        logging.error('Failed to find noise level for %s at %d MHz.',
                      self._name, frequency_mhz)
        return None


def get_prioritized_default_route(host=None, interface_name_regex=None):
    """
    Query a local or remote host for its prioritized default interface
    and route.

    @param interface_name_regex string regex to filter routes by interface.
    @return DefaultRoute tuple, or None if no default routes are found.

    """
    # Build a list of default routes, filtered by interface if requested.
    # Example command output: 'default via 172.23.188.254 dev eth0  metric 2'
    run = host.run if host is not None else utils.run
    output = run('ip route show').stdout
    output_regex_str = 'default\s+via\s+(\S+)\s+dev\s+(\S+)\s+metric\s+(\d+)'
    output_regex = re.compile(output_regex_str)
    defaults = []
    for item in output.splitlines():
        if 'default' not in item:
            continue
        match = output_regex.match(item.strip())
        if match is None:
            raise error.TestFail('Unexpected route output: %s' % item)
        gateway = match.group(1)
        interface_name = match.group(2)
        metric = int(match.group(3))
        if interface_name_regex is not None:
            if re.match(interface_name_regex, interface_name) is None:
                continue
        defaults.append(DefaultRoute(interface_name=interface_name,
                                     gateway=gateway, metric=metric))
    if not defaults:
        return None

    # Sort and return the route with the lowest metric value.
    defaults.sort(key=lambda x: x.metric)
    return defaults[0]

