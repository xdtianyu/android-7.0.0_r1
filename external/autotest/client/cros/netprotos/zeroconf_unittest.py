# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

import dpkt
import fake_host
import socket
import zeroconf


FAKE_HOSTNAME = 'fakehost1'

FAKE_IPADDR = '192.168.11.22'


class TestZeroconfDaemon(unittest.TestCase):
    """Test class for ZeroconfDaemon."""

    def setUp(self):
        self._host = fake_host.FakeHost(FAKE_IPADDR)
        self._zero = zeroconf.ZeroconfDaemon(self._host, FAKE_HOSTNAME)


    def _query_A(self, name):
        """Returns the list of A records matching the given name.

        @param name: A domain name.
        @return a list of dpkt.dns.DNS.RR objects, one for each matching record.
        """
        q = dpkt.dns.DNS.Q(name=name, type=dpkt.dns.DNS_A)
        return self._zero._process_A(q)


    def testRegisterService(self):
        """Tests that we get appropriate records after registering a service."""
        SERVICE_PORT = 9
        SERVICE_TXT_LIST = ['lies=lies']
        self._zero.register_service('unique_prefix', '_service_type',
                                    '_tcp', SERVICE_PORT, SERVICE_TXT_LIST)
        name = '_service_type._tcp.local'
        fq_name = 'unique_prefix.' + name
        # Issue SRV, PTR, and TXT queries
        q_srv = dpkt.dns.DNS.Q(name=fq_name, type=dpkt.dns.DNS_SRV)
        q_txt = dpkt.dns.DNS.Q(name=fq_name, type=dpkt.dns.DNS_TXT)
        q_ptr = dpkt.dns.DNS.Q(name=name, type=dpkt.dns.DNS_PTR)
        ptr_responses = self._zero._process_PTR(q_ptr)
        srv_responses = self._zero._process_SRV(q_srv)
        txt_responses = self._zero._process_TXT(q_txt)
        self.assertTrue(ptr_responses)
        self.assertTrue(srv_responses)
        self.assertTrue(txt_responses)
        ptr_resp = ptr_responses[0]
        srv_resp = [resp for resp in srv_responses
                    if resp.type == dpkt.dns.DNS_SRV][0]
        txt_resp = txt_responses[0]
        # Check that basic things are right.
        self.assertEqual(fq_name, ptr_resp.ptrname)
        self.assertEqual(FAKE_HOSTNAME + '.' + self._zero.domain,
                         srv_resp.srvname)
        self.assertEqual(SERVICE_PORT, srv_resp.port)
        self.assertEqual(SERVICE_TXT_LIST, txt_resp.text)


    def testProperties(self):
        """Test the initial properties set by the constructor."""
        self.assertEqual(self._zero.host, self._host)
        self.assertEqual(self._zero.hostname, FAKE_HOSTNAME)
        self.assertEqual(self._zero.domain, 'local') # Default domain
        self.assertEqual(self._zero.full_hostname, FAKE_HOSTNAME + '.local')


    def testSocketInit(self):
        """Test that the constructor listens for mDNS traffic."""

        # Should create an UDP socket and bind it to the mDNS address and port.
        self.assertEqual(len(self._host._sockets), 1)
        sock = self._host._sockets[0]

        self.assertEqual(sock._family, socket.AF_INET) # IPv4
        self.assertEqual(sock._sock_type, socket.SOCK_DGRAM) # UDP

        # Check it is listening for UDP packets on the mDNS address and port.
        self.assertTrue(sock._bound)
        self.assertEqual(sock._bind_ip_addr, '224.0.0.251') # mDNS address
        self.assertEqual(sock._bind_port, 5353) # mDNS port
        self.assertTrue(callable(sock._bind_recv_callback))


    def testRecordsInit(self):
        """Test the A record of the host is registered."""
        host_A = self._query_A(self._zero.full_hostname)
        self.assertGreater(len(host_A), 0)

        record = host_A[0]
        # Check the hostname and the packed IP address.
        self.assertEqual(record.name, self._zero.full_hostname)
        self.assertEqual(record.ip, socket.inet_aton(self._host.ip_addr))


    def testDoubleTXTProcessing(self):
        """Test when more than one TXT record is present in a packet.

        A mDNS packet can include several answer records for several domains and
        record type. A corner case found on the field presents a mDNS packet
        with two TXT records for the same domain name on the same packet on its
        authoritative answers section while the packet itself is a query.
        """
        # Build the mDNS packet with two TXT records.
        domain_name = 'other_host.local'
        answers = [
                dpkt.dns.DNS.RR(
                        type = dpkt.dns.DNS_TXT,
                        cls = dpkt.dns.DNS_IN,
                        ttl = 120,
                        name = domain_name,
                        text = ['one', 'two']),
                dpkt.dns.DNS.RR(
                        type = dpkt.dns.DNS_TXT,
                        cls = dpkt.dns.DNS_IN,
                        ttl = 120,
                        name = domain_name,
                        text = ['two'])]
        # The packet is a query packet, with extra answers on the autoritative
        # section.
        mdns = dpkt.dns.DNS(
                op = dpkt.dns.DNS_QUERY, # Standard query
                rcode = dpkt.dns.DNS_RCODE_NOERR,
                q = [],
                an = [],
                ns = answers)

        # Record the new answers received on the answer_calls list.
        answer_calls = []
        self._zero.add_answer_observer(lambda args: answer_calls.extend(args))

        # Send the packet to the registered callback.
        sock = self._host._sockets[0]
        cbk = sock._bind_recv_callback
        cbk(str(mdns), '1234', 5353)

        # Check that the answers callback is called with all the answers in the
        # received order.
        self.assertEqual(len(answer_calls), 2)
        ans1, ans2 = answer_calls # Each ans is a (rrtype, rrname, data)
        self.assertEqual(ans1[2], ('one', 'two'))
        self.assertEqual(ans2[2], ('two',))

        # Check that the two records were cached.
        records = self._zero.cached_results(domain_name, dpkt.dns.DNS_TXT)
        self.assertEqual(len(records), 2)


if __name__ == '__main__':
    unittest.main()
