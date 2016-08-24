# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


class FakeHost(object):
    """A fake implementation of a lansim.host.Host object.

    This class replaces the real Host class and should be used for unit testing.
    """

    def __init__(self, ip_addr):
        self.ip_addr = ip_addr

        # List of FakeSocket objects returned by socket()
        self._sockets = []


    def socket(self, family, sock_type):
        """Creates a new FakeSocket and returns it.

        @param family: The socket family, for example AF_INET.
        @param sock_type: The socket type, for example SOCK_DGRAM.
        @return: a FakeSocket object.
        """
        sock = FakeSocket(self, family, sock_type)
        self._sockets.append(sock)
        return sock


class FakeSocket(object):
    """A fake socket interface implementation.

    This class implements a fake socket object as returned by the Host.socket()
    method.
    """

    def __init__(self, host, family, sock_type):
        self._host = host
        self._family = family
        self._sock_type = sock_type
        self._bound = False


    def listen(self, ip_addr, port, recv_callback):
        """Bind and listen on the ip_addr:port.

        The fake implementation only stores these value as members of the
        FakeSocket to allow the test inspect those values.

        @param ip_addr: Local destination ip_addr.
        @param port: Local destination port number.
        @param recv_callback: A callback function that accepts three
        arguments, the received string, the sender IPv4 address and the
        sender port number.
        """
        self._bound = True
        self._bind_ip_addr = ip_addr
        self._bind_port = port
        self._bind_recv_callback = recv_callback

