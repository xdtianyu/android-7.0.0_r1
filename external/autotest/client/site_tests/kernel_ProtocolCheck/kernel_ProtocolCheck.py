# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, socket
from collections import namedtuple

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error

PROTO_FILE = '/proc/net/protocols'

# Defines that python's socket lacks.
IPPROTO_UDPLITE = 193
AF_BLUETOOTH = 31
BTPROTO_L2CAP = 0
BTPROTO_HCI = 1
BTPROTO_SCO = 2
BTPROTO_RFCOMM = 3

# Contains information needed to create a socket using a particular
# protocol.
Protocol = namedtuple('Protocol', ['name', 'domain', 'socket_type',
                                   'proto_num'])
REQUIRED = set([
        Protocol('RFCOMM', AF_BLUETOOTH, socket.SOCK_STREAM, BTPROTO_RFCOMM),
        Protocol('RFCOMM', AF_BLUETOOTH, socket.SOCK_SEQPACKET, BTPROTO_SCO),
        Protocol('L2CAP', AF_BLUETOOTH, socket.SOCK_STREAM, BTPROTO_L2CAP),
        Protocol('HCI', AF_BLUETOOTH, socket.SOCK_RAW, BTPROTO_HCI),
        Protocol('PACKET', socket.AF_PACKET, socket.SOCK_DGRAM, 0),
        Protocol('RAWv6', socket.AF_INET6, socket.SOCK_RAW, 0),
        Protocol('UDPLITEv6', socket.AF_INET6, socket.SOCK_DGRAM,
                 IPPROTO_UDPLITE),
        Protocol('UDPv6', socket.AF_INET6, socket.SOCK_DGRAM, 0),
        Protocol('TCPv6', socket.AF_INET6, socket.SOCK_STREAM, 0),
        Protocol('UNIX', socket.AF_UNIX, socket.SOCK_STREAM, 0),
        Protocol('UDP-Lite', socket.AF_INET, socket.SOCK_DGRAM,
                 IPPROTO_UDPLITE),
        Protocol('PING', socket.AF_INET, socket.SOCK_DGRAM,
                 socket.IPPROTO_ICMP),
        Protocol('RAW', socket.AF_INET, socket.SOCK_RAW, 0),
        Protocol('UDP', socket.AF_INET, socket.SOCK_DGRAM, 0),
        Protocol('TCP', socket.AF_INET, socket.SOCK_STREAM, 0),
        Protocol('NETLINK', socket.AF_NETLINK, socket.SOCK_DGRAM, 0),
        ])

class kernel_ProtocolCheck(test.test):
    version = 1

    def _try_protocol(self, proto):
        """
        Try to create a socket with the specified protocol.

        @param proto Protocol to use to create a socket.
        """
        try:
            sock = socket.socket(proto.domain, proto.socket_type,
                                 proto.proto_num)
            sock.close()
            logging.info('created socket with protocol %s' % (proto.name))
        except socket.error:
            # We don't really care if it fails, any required module should've
            # been loaded anyways.
            logging.info('failed to create socket with protocol %s' %
                         (proto.name))

    def _get_supported_protocols(self):
        """
        Returns the set of supported protocols from /proc/net/protocols.
        """
        f = open(PROTO_FILE)
        if not f:
            raise error.TestError('failed to open %s' % (PROTO_FILE))
        lines = f.readlines()[1:]
        supported = set(line.split()[0] for line in lines)
        f.close()
        return supported

    def run_once(self):
        """
        Check that the kernel supports all required network protocols.
        """
        for proto in REQUIRED:
            # Opening a socket with a protocol should ensure that all necessary
            # modules get loaded.
            self._try_protocol(proto)

        supported = self._get_supported_protocols()

        # Check that each required protocol is supported.
        required = set(proto.name for proto in REQUIRED)
        failures = required - supported

        # Fail if any protocols were unsupported.
        if failures:
            raise error.TestFail('required protocols are unsupported: %s' %
                                 (", ".join(failures)))
