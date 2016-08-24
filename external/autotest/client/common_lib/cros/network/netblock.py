# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.client.common_lib import error

def from_addr(addr, prefix_len=None):
    """Build a Netblock object.

    @param addr: string IP address with optional prefix length
            (e.g. '192.168.1.1' or '192.168.1.1/24'). If |addr| has no
            prefix length, then use the |prefix_len| parameter.
    @param prefix_len: int number of bits forming the IP subnet prefix for
            |addr|. This value will be preferred to the parsed value if
            |addr| has a prefix length as well. If |addr|
            has no prefix length and |prefix_len| is None, then an error
            will be thrown.

    """
    if addr is None:
        raise error.TestError('netblock.from_addr() expects non-None addr '
                              'parameter.')

    prefix_sep_count = addr.count('/')
    if prefix_sep_count > 1:
        raise error.TestError('Invalid IP address found: "%s".' % addr)

    if prefix_sep_count == 1:
        addr_str, prefix_len_str = addr.split('/')
    else:
        # No prefix separator.  Assume addr looks like '192.168.1.1'
        addr_str = addr
        # Rely on passed in |prefix_len|
        prefix_len_str = None

    if prefix_len is not None and prefix_len_str is not None:
        logging.warning('Ignoring parsed prefix length of %s in favor of '
                        'passed in value %d', prefix_len_str, prefix_len)
    elif prefix_len is not None and prefix_len_str is None:
        pass
    elif prefix_len is None and prefix_len_str is not None:
        prefix_len = int(prefix_len_str)
    else:
        raise error.TestError('Cannot construct netblock without knowing '
                              'prefix length for addr: "%s".' % addr)

    return Netblock(addr_str, prefix_len)


class Netblock(object):
    """Utility class for transforming netblock address to related strings."""

    @staticmethod
    def _octets_to_addr(octets):
        """Transform a list of bytes into a string IP address.

        @param octets list of ints (e.g. [192.168.0.1]).
        @return string IP address (e.g. '192.168.0.1.').

        """
        return '.'.join(map(str, octets))


    @staticmethod
    def _int_to_octets(num):
        """Tranform a 32 bit number into a list of 4 octets.

        @param num: number to convert to octets.
        @return list of int values <= 8 bits long.

        """
        return [(num >> s) & 0xff for s in (24, 16, 8, 0)]


    @property
    def netblock(self):
        """@return the IPv4 address/prefix, e.g., '192.168.0.1/24'."""
        return '/'.join([self._octets_to_addr(self._octets),
                         str(self.prefix_len)])


    @property
    def netmask(self):
        """@return the IPv4 netmask, e.g., '255.255.255.0'."""
        return self._octets_to_addr(self._mask_octets)


    @property
    def prefix_len(self):
        """@return the IPv4 prefix len, e.g., 24."""
        return self._prefix_len


    @property
    def subnet(self):
        """@return the IPv4 subnet, e.g., '192.168.0.0'."""
        octets = [a & m for a, m in zip(self._octets, self._mask_octets)]
        return self._octets_to_addr(octets)


    @property
    def broadcast(self):
        """@return the IPv4 broadcast address, e.g., '192.168.0.255'."""
        octets = [a | (m ^ 0xff)
                  for a, m in zip(self._octets, self._mask_octets)]
        return self._octets_to_addr(octets)


    @property
    def addr(self):
        """@return the IPv4 address, e.g., '192.168.0.1'."""
        return self._octets_to_addr(self._octets)


    def __init__(self, addr_str, prefix_len):
        """Construct a Netblock.

        @param addr_str: string IP address (e.g. '192.168.1.1').
        @param prefix_len: int length of subnet prefix (e.g. 24).

        """
        self._octets = map(int, addr_str.split('.'))
        mask_bits = (-1 << (32 - prefix_len)) & 0xffffffff
        self._mask_octets = self._int_to_octets(mask_bits)
        self._prefix_len = prefix_len


    def get_addr_in_block(self, offset):
        """Get an address in a subnet.

        For instance if this netblock represents 192.168.0.1/24,
        then get_addr_in_block(5) would return 192.168.0.5.

        @param offset int offset in block, (e.g. 5).
        @return string address (e.g. '192.168.0.5').

        """
        offset = self._int_to_octets(offset)
        octets = [(a & m) + o
                  for a, m, o in zip(self._octets, self._mask_octets, offset)]
        return self._octets_to_addr(octets)
