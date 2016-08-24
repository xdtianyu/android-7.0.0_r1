# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import socket

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error


# From <bits/socket.h> on Linux 3.2, EGLIBC 2.15.
PROTOCOL_FAMILIES_STR = """
#define PF_UNSPEC       0       /* Unspecified.  */
#define PF_LOCAL        1       /* Local to host (pipes and file-domain).  */
#define PF_UNIX         1       /* POSIX name for PF_LOCAL.  */
#define PF_FILE         1       /* Another non-standard name for PF_LOCAL.  */
#define PF_INET         2       /* IP protocol family.  */
#define PF_AX25         3       /* Amateur Radio AX.25.  */
#define PF_IPX          4       /* Novell Internet Protocol.  */
#define PF_APPLETALK    5       /* Appletalk DDP.  */
#define PF_NETROM       6       /* Amateur radio NetROM.  */
#define PF_BRIDGE       7       /* Multiprotocol bridge.  */
#define PF_ATMPVC       8       /* ATM PVCs.  */
#define PF_X25          9       /* Reserved for X.25 project.  */
#define PF_INET6        10      /* IP version 6.  */
#define PF_ROSE         11      /* Amateur Radio X.25 PLP.  */
#define PF_DECnet       12      /* Reserved for DECnet project.  */
#define PF_NETBEUI      13      /* Reserved for 802.2LLC project.  */
#define PF_SECURITY     14      /* Security callback pseudo AF.  */
#define PF_KEY          15      /* PF_KEY key management API.  */
#define PF_NETLINK      16
#define PF_ROUTE        16      /* Alias to emulate 4.4BSD.  */
#define PF_PACKET       17      /* Packet family.  */
#define PF_ASH          18      /* Ash.  */
#define PF_ECONET       19      /* Acorn Econet.  */
#define PF_ATMSVC       20      /* ATM SVCs.  */
#define PF_RDS          21      /* RDS sockets.  */
#define PF_SNA          22      /* Linux SNA Project */
#define PF_IRDA         23      /* IRDA sockets.  */
#define PF_PPPOX        24      /* PPPoX sockets.  */
#define PF_WANPIPE      25      /* Wanpipe API sockets.  */
#define PF_LLC          26      /* Linux LLC.  */
#define PF_CAN          29      /* Controller Area Network.  */
#define PF_TIPC         30      /* TIPC sockets.  */
#define PF_BLUETOOTH    31      /* Bluetooth sockets.  */
#define PF_IUCV         32      /* IUCV sockets.  */
#define PF_RXRPC        33      /* RxRPC sockets.  */
#define PF_ISDN         34      /* mISDN sockets.  */
#define PF_PHONET       35      /* Phonet sockets.  */
#define PF_IEEE802154   36      /* IEEE 802.15.4 sockets.  */
#define PF_CAIF         37      /* CAIF sockets.  */
#define PF_ALG          38      /* Algorithm sockets.  */
#define PF_NFC          39      /* NFC sockets.  */
#define PF_MAX          40      /* For now...  */
"""

PROTOCOL_FAMILIES = dict([(int(line.split()[2]), line.split()[1])
                          for line
                          in PROTOCOL_FAMILIES_STR.strip().splitlines()])

SOCKET_TYPES = [socket.SOCK_STREAM, socket.SOCK_DGRAM, socket.SOCK_RAW,
                socket.SOCK_RDM, socket.SOCK_SEQPACKET]


class security_ProtocolFamilies(test.test):
    version = 1
    PF_BASELINE = ["PF_FILE", "PF_PACKET", "PF_INET", "PF_INET6", "PF_ROUTE",
                   "PF_LOCAL", "PF_NETLINK", "PF_UNIX", "PF_BLUETOOTH", "PF_ALG"]
    PER_BOARD = {}


    def pf_name(self, pfn):
        protocol_family = ""

        if pfn in PROTOCOL_FAMILIES:
            protocol_family = PROTOCOL_FAMILIES[pfn]
        else:
            protocol_family = "PF %d (unknown)" % pfn

        return protocol_family


    def is_protocol_family_available(self, pfn):
        """Tries to create a socket with protocol family number |pfn|
        and every possible socket type.
        Returns |True| if any socket can be created, |False| otherwise.
        """

        available = False

        for socket_type in SOCKET_TYPES:
            try:
                socket.socket(pfn, socket_type)
                available = True
                logging.info("%s available with socket type %d" %
                             (self.pf_name(pfn), socket_type))
                break
            except socket.error:
                pass

        return available


    def run_once(self):
        """Tries to create a socket with every possible combination of
        protocol family and socket type.
        Fails if it can create a socket for one or more protocol families
        not in the baseline.
        """

        unexpected_protocol_families = []

        # Protocol families currently go up to 40, but this way we make sure
        # to catch new families that might get added to the kernel.
        for pfn in range(256):
            pf_available = self.is_protocol_family_available(pfn)
            protocol_family = self.pf_name(pfn)

            if pf_available:
                # If PF is in baseline, continue.
                if protocol_family in self.PF_BASELINE:
                    continue

                # Check the board-specific whitelist.
                current_board = utils.get_current_board()
                board_pfs = self.PER_BOARD.get(current_board, None)
                if not board_pfs or protocol_family not in board_pfs:
                    unexpected_protocol_families.append(protocol_family)

        if len(unexpected_protocol_families) > 0:
            failure_string = "Unexpected protocol families available: "
            failure_string += ", ".join(unexpected_protocol_families)
            logging.error(failure_string)
            raise error.TestFail(failure_string)
