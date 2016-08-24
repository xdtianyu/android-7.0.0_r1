# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
netprotos - Network Protocols

This module includes Python implementations of various network protocols to
use in testing. These protocols can be used either with the kernel network
stack openning UDP, TCP or RAW sockets or with the simulated network stack
based on the lansim package.

In either case, the interface used requires a Host object providing the
following interface:
 * Host.ip_addr property, with the host's IP address in plain text.
 * Host.socket(family, sock_type) that returns a new asynchronous socket.

An asynchronous socket must have the following interface:
 * listen(ip_addr, port, recv_callback)
 * send(data, ip_addr, port)
 * close()

See lansim.host.UDPSocket for details on the utilization of this interface.
Note that this interface is asynchronous since there's no blocking recv()
method. Instead, when the main loop event handler receives a packet for
this socket, the recv_callback passed will be called.


To create new protocols you can follow the example of ZeroconfDaemon and
CrosP2PDaemon which implement part of those protocols to serve files on
the LAN.

To launch a ZeroconfDaemon on a simulated Host, simply create that
object for the given Host instance as follows:

    zero = zeroconf.ZeroconfDaemon(host_b, "host-name-b")

Once again, a CrosP2PDaemon requires a ZeroconfDaemon instance to
interact with, so simply creating the object will make it available.
Although it is not sharing any file it anounces the num_connections
attribute among other mDNS records required for P2P to work.

    p2p = cros_p2p.CrosP2PDaemon(zero)

To add files and share them on the P2P server, the interface is the
following:

    p2p.add_file('some_payload', 3000)
    p2p.add_file('other_payload', 6000)

"""
