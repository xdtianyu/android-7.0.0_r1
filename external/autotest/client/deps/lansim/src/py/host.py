# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import dpkt
import socket
import struct

from lansim import tools


# An initial set of Protocol to Hardware address mappings.
_ARP_INITIAL_CACHE = {
    # Broadcast address:
    socket.inet_aton('255.255.255.255'): tools.inet_hwton('FF:FF:FF:FF:FF:FF'),
}


class SimpleHostError(Exception):
    """A SimpleHost generic error."""


class SimpleHost(object):
    """A simple host supporting IPv4.

    This class is useful as a base clase to implement other hosts. It supports
    a single IPv4 address.
    """
    def __init__(self, sim, hw_addr, ip_addr):
        """Creates the host and associates it with the given NetworkBridge.

        @param sim: The Simulator interface where this host lives.
        @param hw_addr: Hex or binary representation of the Ethernet address.
        @param ip_addr: The IPv4 address. For example: "10.0.0.1".
        """
        self._sim = sim
        self._hw_addr = hw_addr
        self._ip_addr = ip_addr
        self._bin_hw_addr = tools.inet_hwton(hw_addr)
        self._bin_ip_addr = socket.inet_aton(ip_addr)
        # arp cache: Protocol to Hardware address resolution cache.
        self._arp_cache = dict(_ARP_INITIAL_CACHE)
        # Reply to broadcast ARP requests.
        rule = {
            "dst": "\xff" * 6, # Broadcast HW addr.
            "arp.pln": 4, # Protocol Addres Length is 4 (IP v4).
            "arp.op": dpkt.arp.ARP_OP_REQUEST,
            "arp.tpa": self._bin_ip_addr}
        sim.add_match(rule, self.arp_request)

        # Reply to unicast ARP requests.
        rule["dst"] = self._bin_hw_addr
        sim.add_match(rule, self.arp_request)

        # Mappings used for TCP traffic forwarding.
        self._tcp_fwd_in = {}
        self._tcp_fwd_out = {}
        self._tcp_fwd_ports = {}

    @property
    def ip_addr(self):
        """Returns the host IPv4 address."""
        return self._ip_addr


    @property
    def simulator(self):
        """Returns the Simulator instance where this host runs on."""
        return self._sim


    def arp_request(self, pkt):
        """Sends the ARP_REPLY matching the request.

        @param pkt: a dpkt.Packet with the ARP_REQUEST.
        """
        # Update the local ARP cache whenever we get a request.
        self.add_arp(hw_addr=pkt.arp.sha, ip_addr=pkt.arp.spa)

        arp_resp = dpkt.arp.ARP(
            op = dpkt.arp.ARP_OP_REPLY,
            pln = 4,
            tpa = pkt.arp.spa, # Target Protocol Address.
            tha = pkt.arp.sha, # Target Hardware Address.
            spa = self._bin_ip_addr, # Source Protocol Address.
            sha = self._bin_hw_addr) # Source Hardware Address.
        eth_resp = dpkt.ethernet.Ethernet(
            dst = pkt.arp.sha,
            src = self._bin_hw_addr,
            type = dpkt.ethernet.ETH_TYPE_ARP,
            data = arp_resp)
        self._sim.write(eth_resp)


    def add_arp(self, hw_addr, ip_addr):
        """Maps the ip_addr to a given hw_addr.

        This is useful to send IP packets with send_ip() to hosts that haven't
        comunicate with us yet.

        @param hw_addr: The network encoded corresponding Ethernet address.
        @param ip_addr: The network encoded IPv4 address.
        """
        self._arp_cache[ip_addr] = hw_addr


    def _resolve_mac_address(self, ip_addr):
        """Resolves the hw_addr of an IP address locally when it is known.

        This method uses the information gathered from received ARP requests and
        locally added mappings with add_arp(). It also knows how to resolve
        multicast addresses.

        @param ip_addr: The IP address to resolve encoded in network format.
        @return: The Hardware address encoded in network format or None
        if unknown.
        @raise SimpleHostError if the MAC address for ip_addr is unknown.
        """
        # From RFC 1112 6.4:
        #  An IP host group address is mapped to an Ethernet multicast address
        #  by placing the low-order 23-bits of the IP address into the low-order
        #  23 bits of the Ethernet multicast address 01-00-5E-00-00-00 (hex).
        #  Because there are 28 significant bits in an IP host group address,
        #  more than one host group address may map to the same Ethernet
        #  multicast address.
        int_ip_addr, = struct.unpack('!I', ip_addr)
        if int_ip_addr & 0xF0000000 == 0xE0000000: # Multicast IP address
            int_hw_ending = int_ip_addr & ((1 << 23) - 1) | 0x5E000000
            return '\x01\x00' + struct.pack('!I', int_hw_ending)
        if ip_addr in self._arp_cache:
            return self._arp_cache[ip_addr]
        # No address found.
        raise SimpleHostError("Unknown destination IP host.")


    def send_ip(self, pkt):
        """Sends an IP packet.

        The source IP address and the hardware layer is automatically filled.
        @param pkt: A dpkg.ip.IP packet.
        @raise SimpleHostError if the MAC address for ip_addr is unknown.
        """
        hw_dst = self._resolve_mac_address(pkt.dst)

        pkt.src = self._bin_ip_addr
        # Set the packet length and force to recompute the checksum.
        pkt.len = len(pkt)
        pkt.sum = 0
        hw_pkt = dpkt.ethernet.Ethernet(
            dst = hw_dst,
            src = self._bin_hw_addr,
            type = dpkt.ethernet.ETH_TYPE_IP,
            data = pkt)
        return self._sim.write(hw_pkt)


    def tcp_forward(self, port, dest_addr, dest_port):
        """Forwards all the TCP/IP traffic on a given port to another host.

        This method makes all the incoming traffic for this host on a particular
        port be redirected to dest_addr:dest_port. This allows us to use the
        kernel's network stack to handle that traffic.

        @param port: The TCP port on this simulated host.
        @param dest_addr: A host IP address on the same network in plain text.
        @param dest_port: The TCP port on the destination host.
        """
        if not self._tcp_fwd_ports:
            # Lazy initialization.
            self._sim.add_match({
                'ip.dst': self._bin_ip_addr,
                'ip.p': dpkt.ip.IP_PROTO_TCP}, self._handle_tcp_forward)

        self._tcp_fwd_ports[port] = socket.inet_aton(dest_addr), dest_port


    def _tcp_pick_port(self, dhost, dport):
        """Picks a new unused source TCP port on the host."""
        for p in range(1024, 65536):
            if (dhost, dport, p) in self._tcp_fwd_out:
                continue
            if p in self._tcp_fwd_ports:
                continue
            return p
        raise SimpleHostError("Too many connections.")


    def _handle_tcp_forward(self, pkt):
        # Source from:
        shost = pkt.ip.src
        sport = pkt.ip.tcp.sport
        dport = pkt.ip.tcp.dport

        ### Handle responses from forwarded traffic back to the sender (out).
        if (shost, sport, dport) in self._tcp_fwd_out:
            fhost, fport, oport = self._tcp_fwd_out[(shost, sport, dport)]
            # Redirect the packet
            pkt.ip.tcp.sport = oport
            pkt.ip.tcp.dport = fport
            pkt.ip.dst = fhost
            pkt.ip.tcp.sum = 0 # Force checksum
            self.send_ip(pkt.ip)
            return

        ### Handle incoming traffic to a local forwarded port (in).
        if dport in self._tcp_fwd_ports:
            # Forward to:
            fhost, fport = self._tcp_fwd_ports[dport]

            ### Check if it is an existing connection.
            # lport: The port from where we send data out.
            if (shost, sport, dport) in self._tcp_fwd_in:
                lport = self._tcp_fwd_in[(shost, sport, dport)]
            else:
                # Pick a new local port on our side.
                lport = self._tcp_pick_port(fhost, fport)
                self._tcp_fwd_in[(shost, sport, dport)] = lport
                self._tcp_fwd_out[(fhost, fport, lport)] = (shost, sport, dport)

            # Redirect the packet
            pkt.ip.tcp.sport = lport
            pkt.ip.tcp.dport = fport
            pkt.ip.dst = fhost
            pkt.ip.tcp.sum = 0 # Force checksum
            self.send_ip(pkt.ip)
            return


    def socket(self, family, sock_type):
        """Creates an asynchronous socket on the simulated host.

        This method creates an asynchronous socket object that can be used to
        receive and send packets. This module only supports UDP sockets.

        @param family: The socket family, only AF_INET is supported.
        @param sock_type: The socket type, only SOCK_DGRAM is supported.
        @return: an UDPSocket object. See UDPSocket documentation for details.
        @raise SimpleHostError if socket family and type is not supported.
        """
        if family != socket.AF_INET:
            raise SimpleHostError("socket family not supported.")
        if sock_type != socket.SOCK_DGRAM:
            raise SimpleHostError("socket type not supported.")

        return UDPSocket(self)


class UDPSocket(object):
    """An asynchronous UDP socket interface.

    This UDP socket interface provides a way to send and received UDP messages
    on an asynchronous way. This means that the socket doesn't have a recv()
    method as a normal socket would have, since the simulation is event driven
    and a callback should not block its execution. See the listen() method to
    see how to receive messages from this socket.

    This interface is used by modules outside lansim to interact with lansim
    in a way that can be ported to other different backends. For example, this
    same interface could be implemented using the Python's socket module and
    the real kernel stack.
    """
    def __init__(self, host):
        """Initializes the UDP socket.

        To be used for receiving packets, listen() must be called.

        @param host: A SimpleHost object.
        """
        self._host = host
        self._sim = host.simulator
        self._port = None


    def __del__(self):
        self.close()


    def listen(self, ip_addr, port, recv_callback):
        """Bind and listen on the ip_addr:port.

        Calls recv_callback(pkt, src_addr, src_port) every time an UDP frame
        is received. src_addr and src_port are passed with the source IPv4
        (as in '192.168.0.2') and the sender port number.

        This function can only be called once, since the socket can't be
        reassigned.

        @param ip_addr: Local destination ip_addr. If None, the Host's IPv4
        address is used, for example '224.0.0.251' or '192.168.0.1'.
        @param port: Local destination port number.
        @param recv_callback: A callback function that accepts three
        arguments, the received string, the sender IPv4 address and the
        sender port number.
        """
        if ip_addr is None:
            ip_addr = self._host.ip_addr()
        self._port = port

        # Binds all the traffic to the provided callback converting the
        # single argument callback to the multiple argument.
        self._sim.add_match({
            "ip.dst": socket.inet_aton(ip_addr),
            "ip.udp.dport": port},
            lambda pkt: recv_callback(pkt.ip.udp.data,
                                      socket.inet_ntoa(pkt.ip.src),
                                      pkt.ip.udp.sport))


    def send(self, data, ip_addr, port):
        """Send an UDP message with the data string to ip_addr:port.

        @param data: Any string small enough to fit in a single UDP packet.
        @param ip_addr: Destination IPv4 address.
        @param port: Destination UDP port number.
        """
        pkt_udp = dpkt.udp.UDP(
            dport = port,
            sport = self._port if self._port != None else 0,
            data = data
        )
        # dpkt doesn't set the Length field on UDP packets according to RFC 768.
        pkt_udp.ulen = len(pkt_udp.pack_hdr()) + len(str(pkt_udp.data))

        pkt_ip = dpkt.ip.IP(
            dst = socket.inet_aton(ip_addr),
            ttl = 255, # The comon value for IP packets.
            off = dpkt.ip.IP_DF, # Don't frag.
            p = dpkt.ip.IP_PROTO_UDP,
            data = pkt_udp
        )
        self._host.send_ip(pkt_ip)


    def close(self):
        """Closes the socket and disconnects the bound callback."""
        #TODO(deymo): Remove the add_match rule added on listen().
