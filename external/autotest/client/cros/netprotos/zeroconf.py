# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import collections
import dpkt
import logging
import socket
import time


DnsRecord = collections.namedtuple('DnsResult', ['rrname', 'rrtype', 'data', 'ts'])

MDNS_IP_ADDR = '224.0.0.251'
MDNS_PORT = 5353

# Value to | to a class value to signal cache flush.
DNS_CACHE_FLUSH = 0x8000

# When considering SRV records, clients are supposed to unilaterally prefer
# numerically lower priorities, then pick probabilistically by weight.
# See RFC2782.
# An arbitrary number that will fit in 16 bits.
DEFAULT_PRIORITY = 500
# An arbitrary number that will fit in 16 bits.
DEFAULT_WEIGHT = 500

def _RR_equals(rra, rrb):
    """Returns whether the two dpkt.dns.DNS.RR objects are equal."""
    # Compare all the members present in either object and on any RR object.
    keys = set(rra.__dict__.keys() + rrb.__dict__.keys() +
               dpkt.dns.DNS.RR.__slots__)
    # On RR objects, rdata is packed based on the other members and the final
    # packed string depends on other RR and Q elements on the same DNS/mDNS
    # packet.
    keys.discard('rdata')
    for key in keys:
        if hasattr(rra, key) != hasattr(rrb, key):
            return False
        if not hasattr(rra, key):
            continue
        if key == 'cls':
          # cls attribute should be masked for the cache flush bit.
          if (getattr(rra, key) & ~DNS_CACHE_FLUSH !=
                getattr(rrb, key) & ~DNS_CACHE_FLUSH):
              return False
        else:
          if getattr(rra, key) != getattr(rrb, key):
              return False
    return True


class ZeroconfDaemon(object):
    """Implements a simulated Zeroconf daemon running on the given host.

    This class implements part of the Multicast DNS RFC 6762 able to simulate
    a host exposing services or consuming services over mDNS.
    """
    def __init__(self, host, hostname, domain='local'):
        """Initializes the ZeroconfDameon running on the given host.

        For the purposes of the Zeroconf implementation, a host must have a
        hostname and a domain that defaults to 'local'. The ZeroconfDaemon will
        by default advertise the host address it is running on, which is
        required by some services.

        @param host: The Host instance where this daemon runs on.
        @param hostname: A string representing the hostname
        """
        self._host = host
        self._hostname = hostname
        self._domain = domain
        self._response_ttl = 60 # Default TTL in seconds.

        self._a_records = {} # Local A records.
        self._srv_records = {} # Local SRV records.
        self._ptr_records = {} # Local PTR records.
        self._txt_records = {} # Local TXT records.

        # dict() of name --> (dict() of type --> (dict() of data --> timeout))
        # For example: _peer_records['somehost.local'][dpkt.dns.DNS_A] \
        #     ['192.168.0.1'] = time.time() + 3600
        self._peer_records = {}

        # Register the host address locally.
        self.register_A(self.full_hostname, host.ip_addr)

        # Attend all the traffic to the mDNS port (unicast, multicast or
        # broadcast).
        self._sock = host.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self._sock.listen(MDNS_IP_ADDR, MDNS_PORT, self._mdns_request)

        # Observer list for new responses.
        self._answer_callbacks = []


    def __del__(self):
        self._sock.close()


    @property
    def host(self):
        """The Host object where this daemon is running."""
        return self._host


    @property
    def hostname(self):
        """The hostname part within a domain."""
        return self._hostname


    @property
    def domain(self):
        """The domain where the given hostname is running."""
        return self._domain


    @property
    def full_hostname(self):
        """The full hostname designation including host and domain name."""
        return self._hostname + '.' + self._domain


    def _mdns_request(self, data, addr, port):
        """Handles a mDNS multicast packet.

        This method will generate and send a mDNS response to any query
        for which it has new authoritative information. Called by the Simulator
        as a callback for every mDNS received packet.

        @param data: The string contained on the UDP message.
        @param addr: The address where the message comes from.
        @param port: The port number where the message comes from.
        """
        # Parse the mDNS request using dpkt's DNS module.
        mdns = dpkt.dns.DNS(data)
        if mdns.op == 0x0000: # Query
            QUERY_HANDLERS = {
                dpkt.dns.DNS_A: self._process_A,
                dpkt.dns.DNS_PTR: self._process_PTR,
                dpkt.dns.DNS_TXT: self._process_TXT,
                dpkt.dns.DNS_SRV: self._process_SRV,
            }

            answers = []
            for q in mdns.qd: # Query entries
                if q.type in QUERY_HANDLERS:
                    answers += QUERY_HANDLERS[q.type](q)
                elif q.type == dpkt.dns.DNS_ANY:
                    # Special type matching any known type.
                    for _, handler in QUERY_HANDLERS.iteritems():
                        answers += handler(q)
            # Remove all the already known answers from the list.
            answers = [ans for ans in answers if not any(True
                for known_ans in mdns.an if _RR_equals(known_ans, ans))]

            self._send_answers(answers)

        # Always process the received authoritative answers.
        answers = mdns.ns

        # Process the answers for response packets.
        if mdns.op == 0x8400: # Standard response
            answers.extend(mdns.an)

        if answers:
            cur_time = time.time()
            new_answers = []
            for rr in answers: # Answers RRs
                # dpkt decodes the information on different fields depending on
                # the response type.
                if rr.type == dpkt.dns.DNS_A:
                    data = socket.inet_ntoa(rr.ip)
                elif rr.type == dpkt.dns.DNS_PTR:
                    data = rr.ptrname
                elif rr.type == dpkt.dns.DNS_TXT:
                    data = tuple(rr.text) # Convert the list to a hashable tuple
                elif rr.type == dpkt.dns.DNS_SRV:
                    data = rr.srvname, rr.priority, rr.weight, rr.port
                else:
                    continue # Ignore unsupported records.
                if not rr.name in self._peer_records:
                    self._peer_records[rr.name] = {}
                # Start a new cache or clear the existing if required.
                if not rr.type in self._peer_records[rr.name] or (
                        rr.cls & DNS_CACHE_FLUSH):
                    self._peer_records[rr.name][rr.type] = {}

                new_answers.append((rr.type, rr.name, data))
                cached_ans = self._peer_records[rr.name][rr.type]
                rr_timeout = cur_time + rr.ttl
                # Update the answer timeout if already cached.
                if data in cached_ans:
                    cached_ans[data] = max(cached_ans[data], rr_timeout)
                else:
                    cached_ans[data] = rr_timeout
            if new_answers:
                for cbk in self._answer_callbacks:
                    cbk(new_answers)


    def clear_cache(self):
        """Discards all the cached records."""
        self._peer_records = {}


    def _send_answers(self, answers):
        """Send a mDNS reply with the provided answers.

        This method uses the undelying Host to send an IP packet with a mDNS
        response containing the list of answers of the type dpkt.dns.DNS.RR.
        If the list is empty, no packet is sent.

        @param answers: The list of answers to send.
        """
        if not answers:
            return
        logging.debug('Sending response with answers: %r.', answers)
        resp_dns = dpkt.dns.DNS(
            op = dpkt.dns.DNS_AA, # Authoritative Answer.
            rcode = dpkt.dns.DNS_RCODE_NOERR,
            an = answers)
        # This property modifies the "op" field:
        resp_dns.qr = dpkt.dns.DNS_R, # Response.
        self._sock.send(str(resp_dns), MDNS_IP_ADDR, MDNS_PORT)


    ### RFC 2782 - RR for specifying the location of services (DNS SRV).
    def register_SRV(self, service, proto, priority, weight, port):
        """Publishes the SRV specified record.

        A SRV record defines a service on a port of a host with given properties
        like priority and weight. The service has a name of the form
        "service.proto.domain". The target host, this is, the host where the
        announced service is running on is set to the host where this zeroconf
        daemon is running, "hostname.domain".

        @param service: A string with the service name.
        @param proto: A string with the protocol name, for example "_tcp".
        @param priority: The service priority number as defined by RFC2782.
        @param weight: The service weight number as defined by RFC2782.
        @param port: The port number where the service is running on.
        """
        srvname = service + '.' + proto + '.' + self._domain
        self._srv_records[srvname] = priority, weight, port


    def _process_SRV(self, q):
        """Process a SRV query provided in |q|.

        @param q: The dns.DNS.Q query object with type dpkt.dns.DNS_SRV.
        @return: A list of dns.DNS.RR responses to the provided query that can
        be empty.
        """
        if not q.name in self._srv_records:
            return []
        priority, weight, port = self._srv_records[q.name]
        full_hostname = self._hostname + '.' + self._domain
        ans = dpkt.dns.DNS.RR(
            type = dpkt.dns.DNS_SRV,
            cls = dpkt.dns.DNS_IN | DNS_CACHE_FLUSH,
            ttl = self._response_ttl,
            name = q.name,
            srvname = full_hostname,
            priority = priority,
            weight = weight,
            port = port)
        # The target host (srvname) requires to send an A record with its IP
        # address. We do this as if a query for it was sent.
        a_qry = dpkt.dns.DNS.Q(name=full_hostname, type=dpkt.dns.DNS_A)
        return [ans] + self._process_A(a_qry)


    ### RFC 1035 - 3.4.1, Domains Names - A (IPv4 address).
    def register_A(self, hostname, ip_addr):
        """Registers an Address record (A) pointing to the given IP addres.

        Records registered with method are assumed authoritative.

        @param hostname: The full host name, for example, "somehost.local".
        @param ip_addr: The IPv4 address of the host, for example, "192.0.1.1".
        """
        if not hostname in self._a_records:
            self._a_records[hostname] = []
        self._a_records[hostname].append(socket.inet_aton(ip_addr))


    def _process_A(self, q):
        """Process an A query provided in |q|.

        @param q: The dns.DNS.Q query object with type dpkt.dns.DNS_A.
        @return: A list of dns.DNS.RR responses to the provided query that can
        be empty.
        """
        if not q.name in self._a_records:
            return []
        answers = []
        for ip_addr in self._a_records[q.name]:
            answers.append(dpkt.dns.DNS.RR(
                type = dpkt.dns.DNS_A,
                cls = dpkt.dns.DNS_IN | DNS_CACHE_FLUSH,
                ttl = self._response_ttl,
                name = q.name,
                ip = ip_addr))
        return answers


    ### RFC 1035 - 3.3.12, Domain names - PTR (domain name pointer).
    def register_PTR(self, domain, destination):
        """Register a domain pointer record.

        A domain pointer record is simply a pointer to a hostname on the domain.

        @param domain: A domain name that can include a proto name, for
        example, "_workstation._tcp.local".
        @param destination: The hostname inside the given domain, for example,
        "my-desktop".
        """
        if not domain in self._ptr_records:
            self._ptr_records[domain] = []
        self._ptr_records[domain].append(destination)


    def _process_PTR(self, q):
        """Process a PTR query provided in |q|.

        @param q: The dns.DNS.Q query object with type dpkt.dns.DNS_PTR.
        @return: A list of dns.DNS.RR responses to the provided query that can
        be empty.
        """
        if not q.name in self._ptr_records:
            return []
        answers = []
        for dest in self._ptr_records[q.name]:
            answers.append(dpkt.dns.DNS.RR(
                type = dpkt.dns.DNS_PTR,
                cls = dpkt.dns.DNS_IN, # Don't cache flush for PTR records.
                ttl = self._response_ttl,
                name = q.name,
                ptrname = dest + '.' + q.name))
        return answers


    ### RFC 1035 - 3.3.14, Domain names - TXT (descriptive text).
    def register_TXT(self, domain, txt_list, announce=False):
        """Register a TXT record on a domain with given list of strings.

        A TXT record can hold any list of text entries whos format depends on
        the domain. This method replaces any previous TXT record previously
        registered for the given domain.

        @param domain: A domain name that normally can include a proto name and
        a service or host name.
        @param txt_list: A list of strings.
        @param announce: If True, the method will also announce the changes
        on the network.
        """
        self._txt_records[domain] = txt_list
        if announce:
            self._send_answers(self._process_TXT(dpkt.dns.DNS.Q(name=domain)))


    def _process_TXT(self, q):
        """Process a TXT query provided in |q|.

        @param q: The dns.DNS.Q query object with type dpkt.dns.DNS_TXT.
        @return: A list of dns.DNS.RR responses to the provided query that can
        be empty.
        """
        if not q.name in self._txt_records:
            return []
        text_list = self._txt_records[q.name]
        answer = dpkt.dns.DNS.RR(
            type = dpkt.dns.DNS_TXT,
            cls = dpkt.dns.DNS_IN | DNS_CACHE_FLUSH,
            ttl = self._response_ttl,
            name = q.name,
            text = text_list)
        return [answer]


    def register_service(self, unique_prefix, service_type,
                         protocol, port, txt_list):
        """Register a service in the Avahi style.

        Avahi exposes a convenient set of methods for manipulating "services"
        which are a trio of PTR, SRV, and TXT records.  This is a similar
        helper method for our daemon.

        @param unique_prefix: string unique prefix of service (part of the
                              canonical name).
        @param service_type: string type of service (e.g. '_privet').
        @param protocol: string protocol to use for service (e.g. '_tcp').
        @param port: IP port of service (e.g. 53).
        @param txt_list: list of txt records (e.g. ['vers=1.0', 'foo']).
        """
        service_name = '.'.join([unique_prefix, service_type])
        fq_service_name = '.'.join([service_name, protocol, self._domain])
        logging.debug('Registering service=%s on port=%d with txt records=%r',
                      fq_service_name, port, txt_list)
        self.register_SRV(
                service_name, protocol, DEFAULT_PRIORITY, DEFAULT_WEIGHT, port)
        self.register_PTR('.'.join([service_type, protocol, self._domain]),
                          unique_prefix)
        self.register_TXT(fq_service_name, txt_list)


    def cached_results(self, rrname, rrtype, timestamp=None):
        """Return all the cached results for the requested rrname and rrtype.

        This method is used to request all the received mDNS answers present
        on the cache that were valid at the provided timestamp or later.
        Answers received before this timestamp whose TTL isn't long enough to
        make them valid at the timestamp aren't returned. On the other hand,
        answers received *after* the provided timestamp will always be
        considered, even if they weren't known at the provided timestamp point.
        A timestamp of None will return them all.

        This method allows to retrieve "volatile" answers with a TTL of zero.
        According to the RFC, these answers should be only considered for the
        "ongoing" request. To do this, call this method after a few seconds (the
        request timeout) after calling the send_request() method, passing to
        this method the returned timestamp.

        @param rrname: The requested domain name.
        @param rrtype: The DNS record type. For example, dpkt.dns.DNS_TXT.
        @param timestamp: The request timestamp. See description.
        @return: The list of matching records of the form (rrname, rrtype, data,
                 timeout).
        """
        if timestamp is None:
            timestamp = 0
        if not rrname in self._peer_records:
            return []
        if not rrtype in self._peer_records[rrname]:
            return []
        res = []
        for data, data_ts in self._peer_records[rrname][rrtype].iteritems():
            if data_ts >= timestamp:
                res.append(DnsRecord(rrname, rrtype, data, data_ts))
        return res


    def send_request(self, queries):
        """Sends a request for the provided rrname and rrtype.

        All the known and valid answers for this request will be included in the
        non authoritative list of known answers together with the request. This
        is recommended by the RFC and avoid unnecessary responses.

        @param queries: A list of pairs (rrname, rrtype) where rrname is the
        domain name you are requesting for and the rrtype is the DNS record
        type. For example, ('somehost.local', dpkt.dns.DNS_ANY).
        @return: The timestamp where this request is sent. See cached_results().
        """
        queries = [dpkt.dns.DNS.Q(name=rrname, type=rrtype)
                for rrname, rrtype in queries]
        # TODO(deymo): Inlcude the already known answers on the request.
        answers = []
        mdns = dpkt.dns.DNS(
            op = dpkt.dns.DNS_QUERY,
            qd = queries,
            an = answers)
        self._sock.send(str(mdns), MDNS_IP_ADDR, MDNS_PORT)
        return time.time()


    def add_answer_observer(self, callback):
        """Adds the callback to the list of observers for new answers.

        @param callback: A callable object accepting a list of tuples (rrname,
        rrtype, data) where rrname is the domain name, rrtype the DNS record
        type and data is the information associated with the answers, similar to
        what cached_results() returns.
        """
        self._answer_callbacks.append(callback)
