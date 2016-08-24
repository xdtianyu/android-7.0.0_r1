#!/usr/bin/python
# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Wrapper for an RF switch built on an Elexol EtherIO24.

The EtherIO is documented at
        http://www.elexol.com/IO_Modules/Ether_IO_24_Dip_R.php

This file is both a python module and a command line utility to speak
to the module
"""

import cellular_logging
import collections
import socket
import struct
import sys

log = cellular_logging.SetupCellularLogging('ether_io_rf_switch')


class Error(Exception):
    pass


class EtherIo24(object):
    """Encapsulates an EtherIO24 UDP-GPIO bridge."""

    def __init__(self, hostname, port=2424):
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.socket.bind(('', 0))
        self.destination = (hostname, port)
        self.socket.settimeout(3)   # In seconds

    def SendPayload(self, payload):
        self.socket.sendto(payload, self.destination)

    def SendOperation(self, opcode, list_bytes):
        """Sends the specified opcode with [list_bytes] as an argument."""
        payload = opcode + struct.pack(('=%dB' % len(list_bytes)), *list_bytes)
        self.SendPayload(payload)
        return payload

    def SendCommandVerify(self, write_opcode, list_bytes, read_opcode=None):
        """Sends opcode and bytes,
        then reads to make sure command was executed."""
        if read_opcode is None:
            read_opcode = write_opcode.lower()
        for _ in xrange(3):
            write_sent = self.SendOperation(write_opcode, list_bytes)
            self.SendOperation(read_opcode, list_bytes)
            try:
                response = self.AwaitResponse()
                if response == write_sent:
                    return
                else:
                    log.warning('Unexpected reply:  sent %s, got %s',
                                write_sent.encode('hex_codec'),
                                response.encode('hex_codec'))
            except socket.timeout:
                log.warning('Timed out awaiting reply for %s', write_opcode)
                continue
        raise Error('Failed to execute %s' % write_sent.encode('hex_codec'))

    def AwaitResponse(self):
        (response, address) = self.socket.recvfrom(65536)
        if (socket.gethostbyname(address[0]) !=
                socket.gethostbyname(self.destination[0])):
            log.warning('Unexpected reply source: %s (expected %s)',
                        address, self.destination)
        return response


class RfSwitch(object):
    """An RF switch hooked to an Elexol EtherIO24."""

    def __init__(self, ip):
        self.io = EtherIo24(ip)
        # Must run on pythons without 0bxxx notation.  These are 1110,
        # 1101, 1011, 0111
        decode = [0xe, 0xd, 0xb, 0x7]

        self.port_mapping = []
        for upper in xrange(3):
            for lower in xrange(4):
                self.port_mapping.append(decode[upper] << 4 | decode[lower])

    def SelectPort(self, n):
        """Connects port n to the RF generator."""
        # Set all pins to output

        # !A0:  all pins output
        self.io.SendCommandVerify('!A', [0])
        self.io.SendCommandVerify('A', [self.port_mapping[n]])

    def Query(self):
        """Returns (binary port status, selected port, port direction)."""
        self.io.SendOperation('!a', [])
        raw_direction = self.io.AwaitResponse()
        direction = ord(raw_direction[2])

        self.io.SendOperation('a', [])
        status = ord(self.io.AwaitResponse()[1])
        try:
            port = self.port_mapping.index(status)
        except ValueError:
            port = None

        return status, port, direction


def CommandLineUtility(arguments):
    """Command line utility to control a switch."""

    def Select(switch, remaining_args):
        switch.SelectPort(int(remaining_args.popleft()))

    def Query(switch, unused_remaining_args):
        (raw_status, port, direction) = switch.Query()
        if direction != 0x00:
            print 'Warning: Direction register is %x, should be 0x00' % \
                  direction
        if port is None:
            port_str = 'Invalid'
        else:
            port_str = str(port)
        print 'Port %s  (0x%x)' % (port_str, raw_status)

    def Usage():
        print 'usage:  %s hostname {query|select portnumber}' % sys.argv[0]
        exit(1)

    try:
        hostname = arguments.popleft()
        operation = arguments.popleft()

        switch = RfSwitch(hostname)

        if operation == 'query':
            Query(switch, arguments)
        elif operation == 'select':
            Select(switch, arguments)
        else:
            Usage()
    except IndexError:
        Usage()

if __name__ == '__main__':
    CommandLineUtility(collections.deque(sys.argv[1:]))
