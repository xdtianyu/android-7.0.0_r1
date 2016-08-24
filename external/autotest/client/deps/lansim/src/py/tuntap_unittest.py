# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

from lansim import tuntap


class TunTapTest(unittest.TestCase):
    """Unit tests for the TunTap class."""

    def testCreateTapDevice(self):
        """Tests creation of a TAP device and its attributes."""
        tap = tuntap.TunTap(tuntap.IFF_TAP, name="faketap%d")

        self.assertEqual(tap.mode, tuntap.IFF_TAP)

        # Interface name respects the provided format.
        self.assertTrue(hasattr(tap, 'name'))
        self.assertTrue(tap.name.startswith('faketap'))

        # MTU is set for the interface.
        self.assertTrue(hasattr(tap, 'mtu'))
        self.assertTrue(tap.mtu)


    def testCreateTunDevice(self):
        """Tests creation of a TAP device and its attributes."""
        tun = tuntap.TunTap(tuntap.IFF_TUN, name="faketun%d")
        self.assertEqual(tun.mode, tuntap.IFF_TUN)


    def testTapDeviceHWAddr(self):
        """Tests that we can get and set the HW address of a TAP device."""
        tap = tuntap.TunTap(tuntap.IFF_TAP, name="faketap%d")
        family, addr = tap.get_hwaddr()
        self.assertEqual(family, 1) # Ethernet address

        # Select a different hwaddr.
        addr = addr[:-2] + ('11' if addr[-2:] != '11' else '22')

        new_family, new_addr = tap.set_hwaddr(addr)
        self.assertEqual(new_family, 1)
        self.assertEqual(new_addr, addr)

        new_family, new_addr = tap.get_hwaddr()
        self.assertEqual(new_family, 1)
        self.assertEqual(new_addr, addr)


    def testTapDeviceUpDown(self):
        """Tests if it is possible to bring up and down the interface."""
        tap = tuntap.TunTap(tuntap.IFF_TAP, name="faketap%d")
        # Set the IP address to a safe value:
        tap.set_addr('169.254.10.1')
        self.assertEqual(tap.addr, '169.254.10.1')
        tap.set_addr('0.0.0.0')

        self.assertFalse(tap.is_up())
        tap.up()
        self.assertTrue(tap.is_up())
        # Checks that calling up() twice is harmless.
        tap.up()
        self.assertTrue(tap.is_up())
        tap.down()
        self.assertFalse(tap.is_up())


if __name__ == '__main__':
    unittest.main()

