# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os
import sys

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import p2p_utils
from autotest_lib.client.cros.netprotos import cros_p2p, zeroconf


class p2p_ShareFiles(test.test):
    """The P2P Server class tester.

    This class runs the p2p service (p2p-server and p2p-http-server) and checks
    that the DUT is sharing the files on the network.
    """
    version = 1

    def setup(self):
        self.job.setup_dep(['lansim'])


    def initialize(self):
        dep = 'lansim'
        dep_dir = os.path.join(self.autodir, 'deps', dep)
        logging.info('lansim is at %s', dep_dir)
        self.job.install_pkg(dep, 'dep', dep_dir)

        # Import the lansim modules installed on lansim/build/
        sys.path.append(os.path.join(dep_dir, 'build'))

        self._p2p = p2p_utils.P2PServerOverTap()


    def cleanup(self):
        self._p2p.cleanup()


    def _run_lansim_loop(self, timeout=None, until=None):
        """Run the Simulator main loop for a given time."""
        try:
            self._sim.run(timeout=timeout, until=until)
        except Exception, e:
            logging.exception('Simulator ended with an exception:')
            raise error.TestError('Simulator ended with an exception: %r' % e)


    def run_once(self):
        from lansim import simulator, host

        # Setup the environment where avahi-daemon runs during the test.
        try:
            self._p2p.setup()
        except:
            logging.exception('Failed to start tested services.')
            raise

        self._sim = simulator.Simulator(self._p2p.tap)
        # Create a single fake peer that will be sending the multicast requests.
        peer = host.SimpleHost(self._sim, '94:EB:2C:00:00:61', '169.254.10.97')

        # Run a userspace implementation of avahi + p2p-client on the fake
        # host. This will use the P2P services exported by the DUT.
        zero = zeroconf.ZeroconfDaemon(peer, 'a-peer')
        p2pcli = cros_p2p.CrosP2PClient(zero)

        # On p2p-server startup, it should announce the service even if we
        # aren't sharing any file. Usually it doesn't take more than 2 seconds
        # to start announcing the service, repeated a few times.
        self._run_lansim_loop(timeout=20, until=p2pcli.get_peers())
        # Check that we see the DUT on the list of peers.
        peers = p2pcli.get_peers()
        if len(peers) != 1:
            logging.info('Found peers: %r', peers)
            raise error.TestFail('Expected one peer (the DUT) but %d found.' %
                                 len(peers))

        # Check that the announced information is correct.
        peer_name, _hostname, ips, port = peers[0]
        if len(ips) != 1 or ips[0] != self._p2p.tap.addr:
            logging.info('Peer ips: %r', ips)
            raise error.TestFail('Found wrong peer IP address on the DUT.')
        if port != cros_p2p.CROS_P2P_PORT:
            logging.info('Peer p2p port is: %r', port)
            raise error.TestFail('Found wrong p2p port exported on the DUT.')

        files = p2pcli.get_peer_files(peer_name)
        if files:
            logging.info('Peer files: %r', files)
            raise error.TestFail('Found exported files on the DUT.')

        num_connections = p2pcli.get_peer_connections(peer_name)
        if num_connections:
            logging.info('Peer connections: %r', num_connections)
            raise error.TestFail('DUT already has p2p connections.')

        # Share a small file and check that it is broadcasted.
        with open(os.path.join(p2p_utils.P2P_SHARE_PATH, 'my_file=HASH==.p2p'),
                  'w') as f:
            f.write('0123456789')

        # Run the loop until the file is shared. Normally, the p2p-server takes
        # up to 1 second to detect a change on the shared directory and
        # announces it right away a few times. Wait until the file is announced,
        # what should not take more than a few seconds. If after 30 seconds the
        # files isn't announced, that is an error.
        self._run_lansim_loop(timeout=30,
                              until=lambda: p2pcli.get_peer_files(peer_name))

        files = p2pcli.get_peer_files(peer_name)
        if files != [('my_file=HASH==', 10)]:
            logging.info('Peer files: %r', files)
            raise error.TestFail('Expected exported file on the DUT.')

        # Test that the DUT replies to active requests.
        zero.clear_cache()
        p2pcli.start_query()
        # A query can be replied by several peers after it is send, but there's
        # no one-to-one mapping between these two. A query simply forces other
        # peers to send the requested information shortly after. Thus, here we
        # just wait a few seconds until we decide that the query timeouted.
        self._run_lansim_loop(timeout=3)
        p2pcli.stop_query()

        files = p2pcli.get_peer_files(peer_name)
        if files != [('my_file=HASH==', 10)]:
            logging.info('Peer files: %r', files)
            raise error.TestFail('Expected exported file on the DUT.')
