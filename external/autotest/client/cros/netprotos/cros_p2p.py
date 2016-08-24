# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import dpkt
import re


CROS_P2P_PROTO = '_cros_p2p._tcp'
CROS_P2P_PORT = 16725


class CrosP2PDaemon(object):
    """Simulates a P2P server.

    The simulated P2P server will instruct the underlying ZeroconfDaemon to
    reply to requests sharing the files registered on this server.
    """
    def __init__(self, zeroconf, port=CROS_P2P_PORT):
        """Initialize the CrosP2PDaemon.

        @param zeroconf: A ZeroconfDaemon instance where this P2P server will be
        announced.
        @param port: The port where the HTTP server part of the P2P protocol is
        listening. The HTTP server is assumend to be running on the same host as
        the provided ZeroconfDaemon server.
        """
        self._zeroconf = zeroconf
        self._files = {}
        self._num_connections = 0

        self._p2p_domain = CROS_P2P_PROTO + '.' + zeroconf.domain
        # Register the HTTP Server.
        zeroconf.register_SRV(zeroconf.hostname, CROS_P2P_PROTO, 0, 0, port)
        # Register the P2P running on this server.
        zeroconf.register_PTR(self._p2p_domain, zeroconf.hostname)
        self._update_records(False)


    def add_file(self, file_id, file_size, announce=False):
        """Add or update a shared file.

        @param file_id: The name of the file (without .p2p extension).
        @param file_size: The expected total size of the file.
        @param announce: If True, the method will also announce the changes
        on the network.
        """
        self._files[file_id] = file_size
        self._update_records(announce)


    def remove_file(self, file_id, announce=False):
        """Remove a shared file.

        @param file_id: The name of the file (without .p2p extension).
        @param announce: If True, the method will also announce the changes
        on the network.
        """
        del self._files[file_id]
        self._update_records(announce)


    def set_num_connections(self, num_connections, announce=False):
        """Sets the number of connections that the HTTP server is handling.

        This method allows the P2P server to properly announce the number of
        connections it is currently handling.

        @param num_connections: An integer with the number of connections.
        @param announce: If True, the method will also announce the changes
        on the network.
        """
        self._num_connections = num_connections
        self._update_records(announce)


    def _update_records(self, announce):
        # Build the TXT records:
        txts = ['num_connections=%d' % self._num_connections]
        for file_id, file_size in self._files.iteritems():
            txts.append('id_%s=%d' % (file_id, file_size))
        self._zeroconf.register_TXT(
            self._zeroconf.hostname + '.' + self._p2p_domain, txts, announce)


class CrosP2PClient(object):
    """Simulates a P2P client.

    The P2P client interacts with a ZeroconfDaemon instance that inquires the
    network and collects the mDNS responses. A P2P client instance decodes those
    responses according to the P2P protocol implemented over mDNS.
    """
    def __init__(self, zeroconf):
        self._zeroconf = zeroconf
        self._p2p_domain = CROS_P2P_PROTO + '.' + zeroconf.domain
        self._in_query = 0
        zeroconf.add_answer_observer(self._new_answers)


    def start_query(self):
        """Sends queries to gather all the p2p information on the network.

        When a response that requires to send a new query to the peer is
        received, such query will be sent until stop_query() is called.
        Responses received when no query is running will not generate a new.
        """
        self._in_query += 1
        ts = self._zeroconf.send_request([(self._p2p_domain, dpkt.dns.DNS_PTR)])
        # Also send requests for all the known PTR records.
        queries = []


        # The PTR record points to a SRV name.
        ptr_recs = self._zeroconf.cached_results(
                self._p2p_domain, dpkt.dns.DNS_PTR, ts)
        for _rrname, _rrtype, p2p_peer, _deadline in ptr_recs:
            # Request all the information for that peer.
            queries.append((p2p_peer, dpkt.dns.DNS_ANY))
            # The SRV points to a hostname, port, etc.
            srv_recs = self._zeroconf.cached_results(
                    p2p_peer, dpkt.dns.DNS_SRV, ts)
            for _rrname, _rrtype, service, _deadline in srv_recs:
                srvname, _priority, _weight, port = service
                # Request all the information for the host name.
                queries.append((srvname, dpkt.dns.DNS_ANY))
        if queries:
            self._zeroconf.send_request(queries)


    def stop_query(self):
        """Stops a started query."""
        self._in_query -= 1


    def _new_answers(self, answers):
        if not self._in_query:
            return
        queries = []
        for rrname, rrtype, data in answers:
            if rrname == self._p2p_domain and rrtype == dpkt.dns.DNS_PTR:
                # data is a "ptrname" string.
                queries.append((ptrname, dpkt.dns.DNS_ANY))
        if queries:
            self._zeroconf.send_request(queries)


    def get_peers(self, timestamp=None):
        """Return the cached list of peers.

        @param timestamp: The deadline timestamp to consider the responses.
        @return: A list of tuples of the form (peer_name, hostname, list_of_IPs,
                 port).
        """
        res = []
        # The PTR record points to a SRV name.
        ptr_recs = self._zeroconf.cached_results(
                self._p2p_domain, dpkt.dns.DNS_PTR, timestamp)
        for _rrname, _rrtype, p2p_peer, _deadline in ptr_recs:
            # The SRV points to a hostname, port, etc.
            srv_recs = self._zeroconf.cached_results(
                    p2p_peer, dpkt.dns.DNS_SRV, timestamp)
            for _rrname, _rrtype, service, _deadline in srv_recs:
                srvname, _priority, _weight, port = service
                # Each service points to a hostname (srvname).
                a_recs = self._zeroconf.cached_results(
                        srvname, dpkt.dns.DNS_A, timestamp)
                ip_list = [ip for _rrname, _rrtype, ip, _deadline in a_recs]
                res.append((p2p_peer, srvname, ip_list, port))
        return res


    def get_peer_files(self, peer_name, timestamp=None):
        """Returns the cached list of files of the given peer.

        @peer_name: The peer_name as provided by get_peers().
        @param timestamp: The deadline timestamp to consider the responses.
        @return: A list of tuples of the form (file_name, current_size).
        """
        res = []
        txt_records = self._zeroconf.cached_results(
                peer_name, dpkt.dns.DNS_TXT, timestamp)
        for _rrname, _rrtype, txt_list, _deadline in txt_records:
            for txt in txt_list:
                m = re.match(r'^id_(.*)=([0-9]+)$', txt)
                if not m:
                    continue
                file_name, size = m.groups()
                res.append((file_name, int(size)))
        return res


    def get_peer_connections(self, peer_name, timestamp=None):
        """Returns the cached num_connections of the given peer.

        @peer_name: The peer_name as provided by get_peers().
        @param timestamp: The deadline timestamp to consider the responses.
        @return: A list of tuples of the form (file_name, current_size).
        """
        txt_records = self._zeroconf.cached_results(
                peer_name, dpkt.dns.DNS_TXT, timestamp)
        for _rrname, _rrtype, txt_list, _deadline in txt_records:
            for txt in txt_list:
                m = re.match(r'num_connections=(\d+)$', txt)
                if m:
                    return int(m.group(1))
        return None # No num_connections found.
