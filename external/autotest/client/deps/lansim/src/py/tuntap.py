# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import fcntl
import os
import struct
import socket

from lansim import pyiftun
from lansim import tools


# Export some constants used by callers to pass to the |mode| argument while
# create a TunTap() object.
from lansim.pyiftun import IFF_TAP, IFF_TUN


class TunTapError(Exception):
    """TunTap specific error."""


ETHERNET_HEADER_SIZE = 18


STRUCT_IFREQ_FMT = {
    "ifr_flags": "h", # short ifr_flags
    "ifr_mtu": "i", # int ifr_flags
    "ifr_addr": "HH12s", # struct sockaddr_in ifr_addr
    "ifr_hwaddr": "H14s", # struct sockaddr ifru_hwaddr
}


IFNAMSIZ_FMT = str(pyiftun.IFNAMSIZ) + 's'


def pack_struct_ifreq(if_name, argname, *args):
    """Packs a binary string representing a struct ifreq.

    The struct ifreq is used to call ioctl() on network devices. The argument
    type and size depends on the operation performed and is represented as the
    union of different types. This function packs the struct according to the
    provided |argname| which defines the type of |arg|. See netdevice(7) for a
    list of possible arguments and ioctl() commands.

    @param if_name: The interface name.
    @param argname: The name of the member used for the union in struct ifreq.
    @param args: The values used to pack the requested |argname|.
    @raises ValueError: if |argname| isn't a supported union's member name.
    """
    if argname not in STRUCT_IFREQ_FMT:
      raise ValueError()
    return struct.pack(IFNAMSIZ_FMT + STRUCT_IFREQ_FMT[argname], if_name, *args)


def unpack_struct_ifreq(data, argname):
    """Returns a tuple with the interpreted contents of the a struct ifreq.

    The result returned from a ioctl() on network devices has the same format
    than the passed struct ifreq request. This function decodes this result into
    a python tuple, which depends on the |argname| passed to

    @param data: The packed representation of the struct ifreq.
    @param argname: The name of the member used for the union in struct ifreq.
    @raises ValueError: if |argname| isn't a supported union's member name.
    """
    if argname not in STRUCT_IFREQ_FMT:
      raise ValueError()
    return struct.unpack(IFNAMSIZ_FMT + STRUCT_IFREQ_FMT[argname], data)


class TunTap(object):
    """TUN/TAP network interface manipulation class."""


    DEFAULT_DEV_NAME = {
        IFF_TUN: "tun%d",
        IFF_TAP: "tap%d",
    }


    def __init__(self, mode=pyiftun.IFF_TUN, name=None, tundev='/dev/net/tun'):
        """Creates or re-opens a TUN/TAP interface.

        @param mode: This argument is passed to the TUNSETIFF ioctl() to create
        the interface. It says whether the interface created is a TAP (IFF_TAP)
        or TUN (IFF_TUN) interface and some related constant flags found on
        pyiftun.IFF_*.
        @param name: The name of the created interface. If the name ends in '%d'
        that value will be replaced by the kernel with a given number, otherwise
        the name will be appended with '%d'.
        @param tundev: The path to the kerner interface to the tun driver which
        defaults to the standard '/dev/net/tun' if not specified.
        """
        tun_type = mode & pyiftun.TUN_TYPE_MASK
        if tun_type not in self.DEFAULT_DEV_NAME:
            raise TunTapError("mode (%r) not supported" % mode)

        self.mode = mode

        # The interface name can have a "%d" that the kernel will replace with
        # a number.
        if name is None:
            name = self.DEFAULT_DEV_NAME[tun_type]
        elif not name.endswith('%d'):
            name += "%d"

        # Create the TUN/TAP interface.
        fd = os.open(tundev, os.O_RDWR)
        self._fd = fd

        ifs = fcntl.ioctl(fd, pyiftun.TUNSETIFF,
            pack_struct_ifreq(name, 'ifr_flags', mode))
        ifs_name, ifs_mode = struct.unpack(IFNAMSIZ_FMT + "H", ifs)
        self.name = ifs_name.rstrip('\0')

        # Socket used for ioctl() operations over the network device.
        self._sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

        self.mtu = self._get_mtu()


    def __del__(self):
        if hasattr(self, '_fd'):
            os.close(self._fd)


    def _get_mtu(self):
        ifs = fcntl.ioctl(self._sock, pyiftun.SIOCGIFMTU,
            pack_struct_ifreq(self.name, 'ifr_mtu', 0))
        ifr_name, ifr_mtu = unpack_struct_ifreq(ifs, 'ifr_mtu')
        return ifr_mtu


    def _get_flags(self):
        ifs = fcntl.ioctl(self._sock, pyiftun.SIOCGIFFLAGS,
            pack_struct_ifreq(self.name, 'ifr_flags', 0))
        ifr_name, ifr_flags = unpack_struct_ifreq(ifs, 'ifr_flags')
        return ifr_flags


    def _set_flags(self, flags):
        ifs = fcntl.ioctl(self._sock, pyiftun.SIOCSIFFLAGS,
            pack_struct_ifreq(self.name, 'ifr_flags', flags))
        ifr_name, ifr_flags = unpack_struct_ifreq(ifs, 'ifr_flags')
        return ifr_flags


    def get_addr(self):
        """Return the address of the interface.

        @param string addr: The IPv4 address for the interface.
        """
        ifs = fcntl.ioctl(self._sock, pyiftun.SIOCGIFADDR,
            pack_struct_ifreq(self.name, 'ifr_addr', socket.AF_INET, 0, ''))
        ifr_name, ifr_family, ifr_type, ifr_addr = unpack_struct_ifreq(
                ifs, 'ifr_addr')
        if ifr_type != 0:
            return None
        # ifr_addr contains up to 12 bytes (see STRUCT_IFREQ_FMT).
        return socket.inet_ntoa(ifr_addr[:4])


    def set_addr(self, addr, mask=None):
        """Sets the address and network mask of the interface.

        @param string addr: The IPv4 address for the interface.
        """
        str_addr = socket.inet_aton(addr)
        ifs = fcntl.ioctl(self._sock, pyiftun.SIOCSIFADDR,
            pack_struct_ifreq(self.name, 'ifr_addr',
                socket.AF_INET, 0, str_addr))

        if mask != None:
          net_mask = (1 << 32) - (1 << (32 - mask))
          str_mask = struct.pack('!I', net_mask)
          ifs = fcntl.ioctl(self._sock, pyiftun.SIOCSIFNETMASK,
              pack_struct_ifreq(self.name, 'ifr_addr',
                  socket.AF_INET, 0, str_mask))


    """The interface IPv4 address in plain text as in '192.168.0.1'."""
    addr = property(get_addr, set_addr)


    def get_hwaddr(self):
        ifs = fcntl.ioctl(self._sock, pyiftun.SIOCGIFHWADDR,
            pack_struct_ifreq(self.name, 'ifr_hwaddr', 0, ''))
        ifr_name, ifr_family, ifr_hwaddr = unpack_struct_ifreq(
            ifs, 'ifr_hwaddr')
        return (ifr_family, tools.inet_ntohw(ifr_hwaddr[:6]))


    def set_hwaddr(self, hwaddr):
        """Sets the hardware ethernet address of the interface.

        The interface needs to be down in order to set this hardware (MAC)
        address.

        @param string hwaddr: The address in hex format: 'aa:bb:cc:DD:EE:FF'.
        """
        ifs = fcntl.ioctl(self._sock, pyiftun.SIOCSIFHWADDR,
            pack_struct_ifreq(self.name, 'ifr_hwaddr', 1, # 1 for Ethernet
                              tools.inet_hwton(hwaddr)))
        ifr_name, ifr_family, ifr_hwaddr = unpack_struct_ifreq(
            ifs, 'ifr_hwaddr')
        return (ifr_family, tools.inet_ntohw(ifr_hwaddr[:6]))


    """The interface Ethernet address as in '00:11:22:AA:BB:CC'."""
    hwaddr = property(get_hwaddr, set_hwaddr)


    def up(self):
        """Brings up the interface."""
        self._set_flags(self._get_flags() | pyiftun.IFF_UP)


    def down(self):
        """Brings down the interface."""
        self._set_flags(self._get_flags() & ~pyiftun.IFF_UP)


    def is_up(self):
        """Returns whether the interface is up."""
        return (self._get_flags() & pyiftun.IFF_UP) != 0


    def read(self):
        """Reads a 'sent' frame from the interface.

        The frame format depends on the interface type: Ethernet frame for TAP
        interfaces and IP frame for TUN interfaces. This function blocks until
        a new frame is available.

        @return string: A single frame sent to the interface.
        """
        return os.read(self._fd, self.mtu + ETHERNET_HEADER_SIZE)


    def write(self, data):
        """Write a 'received' frame from the interface.

        The frame format depends on the interface type: Ethernet frame for TAP
        interfaces and IP frame for TUN interfaces. This function does not
        block.

        @param data: A single frame received from the interface.
        """
        os.write(self._fd, data)


    def fileno(self):
        """Returns a file descriptor suitable to be used with select()."""
        return self._fd
