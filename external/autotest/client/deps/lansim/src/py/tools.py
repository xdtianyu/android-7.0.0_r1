# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


def inet_hwton(hw_addr):
    """Converts a representation of an Ethernet address to its packed format.

    @param hw_addr: A string representing an Ethernet address in hex format like
    "BA:C0:11:C0:FF:EE", "BAC011C0FFEE" or already in its binary representation.
    @return: The 6-byte binary form of the provided Ethernet address.
    """
    if len(hw_addr) == 6: # Already in network format.
        return hw_addr
    if len(hw_addr) == 12: # Hex format without : in between.
        return ''.join(chr(int(hw_addr[i:i + 2], 16)) for i in range(0, 12, 2))
    if len(hw_addr) == 17: # Hex format with : in between.
        return ''.join(chr(int(hw_addr[i:i + 2], 16)) for i in range(0, 17, 3))


def inet_ntohw(packed_hw_addr):
    """Converts a binary packed Ethernet address to its hex representation.

    @param packed_hw_addr: The 6-byte binary form of the provided Ethernet
    address.
    @return: The hex representation as in "AA:BB:CC:00:11:22".
    """
    return '%.2X:%.2X:%.2X:%.2X:%.2X:%.2X' % tuple(map(ord, packed_hw_addr))
