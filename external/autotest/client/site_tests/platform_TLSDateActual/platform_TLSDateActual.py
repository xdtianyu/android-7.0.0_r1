# Copyright (c) 2013 The Chromium OS Authors and the python-socks5 authors.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 3,
# as published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.

import subprocess
import test

# Taken and hacked from https://code.google.com/p/python-socks5/

import socket
from threading import Thread

SOCKTIMEOUT=5
RESENDTIMEOUT=300

class Forwarder(Thread):
    def __init__(self,src,dest):
        Thread.__init__(self)
        self.src=src
        self.dest=dest

    def __str__(self):
        return '<Forwarder from %s to %s>' % (self.src, self.dest)

    def run(self):
        print '%s: starting' % self
        try:
            self.forward()
        except socket.error as e:
            print '%s: exception %s' % (self, e)
            self.src.close()
            self.dest.close()
        finally:
            print '%s: exiting' % self

    def forward(self):
        BUFSIZE = 1024
        data = self.src.recv(BUFSIZE)
        while data:
            self.dest.sendall(data)
            data = self.src.recv(BUFSIZE)
        self.src.close()
        self.dest.close()
        print '%s: client quit normally' % self

class ProxyForwarder(Forwarder):
    def __init__(self, src, dest_addr):
        Forwarder.__init__(self, src, None)
        self.dest_addr = dest_addr
        self.src = src
        self.dest = None

    def __str__(self):
        return '<ProxyForwarder between %s and %s (%s:%d)' % (
            self.src, self.dest, self.dest_addr[0], self.dest_addr[1])

    def forward(self):
        self.dest = socket.socket(socket.AF_INET,socket.SOCK_STREAM)
        self.dest.connect(self.dest_addr)
        self.src.settimeout(RESENDTIMEOUT)
        self.dest.settimeout(RESENDTIMEOUT)
        Forwarder(self.src,self.dest).start()
        Forwarder(self.dest,self.src).start()

def recvbytes(sock, n):
    bs = sock.recv(n)
    return [ ord(x) for x in bs ]

def recvshort(sock):
    x = recvbytes(sock, 2)
    return x[0] * 256 + x[1]

def create_server(ip,port):
    SOCKS5_VER = "\x05"
    AUTH_NONE = "\x00"

    ATYP_DOMAIN = 0x03

    CMD_CONNECT = 0x01

    ERR_SUCCESS = "\x00"
    ERR_UNSUPP = "\x07"

    transformer = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    transformer.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    transformer.bind((ip, port))
    transformer.listen(1000)

    network_port = chr(port >> 8) + chr(port & 0xff)
    # Turn the textual IP address we were supplied with into a
    # network-byte-order IP address for SOCKS5 wire protocol
    network_ip = "".join(chr(int(i)) for i in ip.split("."))
    while True:
        sock = transformer.accept()[0]
        sock.settimeout(SOCKTIMEOUT)
        print "Got one client connection"
        (_, nmethods) = recvbytes(sock, 2)
        _ = recvbytes(sock, nmethods)
        sock.sendall(SOCKS5_VER + AUTH_NONE)
        (_, cmd, _, atyp) = recvbytes(sock, 4)
        dst_addr = None
        dst_port = None
        if atyp == ATYP_DOMAIN:
            addr_len = recvbytes(sock, 1)[0]
            dst_addr = "".join([unichr(x) for x in recvbytes(sock, addr_len)])
            dst_port = recvshort(sock)
        else:
            socket.sendall(SOCKS5_VER + ERR_UNSUPP + network_ip + network_port)
        print "Proxying to %s:%d" %(dst_addr,dst_port)

        if cmd == CMD_CONNECT:
            sock.sendall(SOCKS5_VER + ERR_SUCCESS + "\x00" + "\x01" +
                         network_ip + network_port)
            print "Starting forwarding thread"
            ProxyForwarder(sock, (dst_addr, dst_port)).start()
        else:
            sock.sendall(SOCKS5_VER + ERR_UNSUPP + network_ip + network_port)
            sock.close()

class ServingThread(Thread):
	def __init__(self, ip, port):
		Thread.__init__(self)
		self.ip = ip
		self.port = port

	def run(self):
		create_server(self.ip, self.port)

class platform_TLSDateActual(test.test):
    version = 1

    def tlsdate(self, host, proxy):
        args = ['/usr/bin/tlsdate', '-v', '-l', '-H', host]
        if proxy:
            args += ['-x', proxy]
        p = subprocess.Popen(args, stderr=subprocess.PIPE)
        out = p.communicate()[1]
        print out
        return p.returncode

    def run_once(self):
        t = ServingThread("127.0.0.1", 8083)
        t.start()
        r = self.tlsdate('clients3.google.com', None)
        if r != 0:
            raise error.TestFail('tlsdate with no proxy to good host failed: %d' % r)
        r = self.tlsdate('clients3.google.com', 'socks5://127.0.0.1:8083')
        if r != 0:
            raise error.TestFail('tlsdate with proxy to good host failed: %d' % r)
        r = self.tlsdate('invalid-host.example.com', None)
        if r == 0:
            raise error.TestFail('tlsdate with no proxy to bad host succeeded')
        r = self.tlsdate('invalid-host.example.com', 'socks5://127.0.0.1:8083')
        if r == 0:
            raise error.TestFail('tlsdate with proxy to bad host succeeded')
