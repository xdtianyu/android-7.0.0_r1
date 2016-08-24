# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os
import sys
import tempfile
import time

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error, utils
from autotest_lib.client.cros import p2p_utils
from autotest_lib.client.cros.netprotos import cros_p2p, zeroconf


class p2p_ServeFiles(test.test):
    """The P2P Server class tester.

    This class runs the p2p service (p2p-server and p2p-http-server) and checks
    that the DUT is serving the shared files on the network.
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
        self._sim = None


    def cleanup(self):
        # Work around problem described in the chromium:364583 bug.
        time.sleep(1)
        self._join_simulator()
        self._p2p.cleanup()


    def _join_simulator(self):
        """Stops the simulator and logs any exception generated there."""
        if not self._sim:
            return
        self._sim.stop()
        self._sim.join()
        if self._sim.error:
            logging.error('SimulatorThread exception: %r', self._sim.error)
            logging.error(self._sim.traceback)


    def _dut_ready(self, p2pcli):
        # Lookup the DUT on the mDNS network.
        peers = p2pcli.get_peers()
        if not peers:
            return False
        peer_name, hostname, ips, port = peers[0]
        # Get the files shared by the DUT.
        files = p2pcli.get_peer_files(peer_name)
        if not files:
            return False
        return peer_name, hostname, ips, port, files


    def _p2p_fetch(self, host, port, filename):
        """Fetch a file from a p2p-http-server.

        @return: A str with the contents of the responde if the request
        succeeds or an integer value with the error code returned by curl
        otherwise.
        """
        fd, tempfn = tempfile.mkstemp(prefix='p2p-fetch')
        ret = utils.run(
                'curl', args=['http://%s:%s/%s' % (host, port, filename)],
                timeout=20., ignore_timeout=False, ignore_status=True,
                stdout_tee=open(tempfn, 'w'), stderr_tee=sys.stdout)
        with os.fdopen(fd) as f:
            output = f.read()
        os.unlink(tempfn)

        if ret is None:
            return None
        if ret.exit_status != 0:
            return ret.exit_status
        return output


    def run_once(self):
        from lansim import simulator, host

        # Setup the environment where avahi-daemon runs during the test.
        try:
            self._p2p.setup(dumpdir=self.job.resultdir)
        except:
            logging.exception('Failed to start tested services.')
            raise

        # Share a file on the DUT.
        content = open('/dev/urandom').read(16*1024)
        with open(os.path.join(p2p_utils.P2P_SHARE_PATH, 'file.p2p'), 'w') as f:
            f.write(content)

        self._sim = simulator.SimulatorThread(self._p2p.tap)
        # Create a single fake peer that will be sending the multicast requests.
        peer = host.SimpleHost(self._sim, '94:EB:2C:00:00:61', '169.254.10.55')

        # Run a userspace implementation of avahi + p2p-client on the fake
        # host. This will use the P2P services exported by the DUT.
        zero = zeroconf.ZeroconfDaemon(peer, 'peer')
        p2pcli = cros_p2p.CrosP2PClient(zero)

        self._sim.start()

        # Force a request from the client before waiting for the DUT's response.
        self._sim.run_on_simulator(lambda: p2pcli.start_query())

        # Wait up to 30 seconds until the DUT is ready sharing the files.
        res = self._sim.wait_for_condition(lambda: self._dut_ready(p2pcli),
                                           timeout=30.)
        self._sim.run_on_simulator(lambda: p2pcli.stop_query())

        if not res:
            raise error.TestFail('The DUT failed to announce the shared files '
                                 'after 30 seconds.')

        # DUT's p2p-http-server is running on hostname:port.
        peer_name, hostname, ips, port, files = res

        if len(files) != 1 or files[0] != ('file', len(content)) or (
                len(ips) != 1) or ips[0] != self._p2p.tap.addr:
            logging.error('peer_name = %r', peer_name)
            logging.error('hostname = %r', hostname)
            logging.error('ips = %r', ips)
            logging.error('port = %r', port)
            logging.error('files = %r', files)
            raise error.TestFail('The DUT announced an erroneous file.')

        # Check we can't download directly from localhost.
        for host_ip in (ips[0], '127.0.0.1'):
            ret = self._p2p_fetch(host_ip, port, 'file')
            if ret != 7: # curl's exit code 7 is "Failed to connect to host."
                logging.error('curl returned: %s', repr(ret)[:100])
                raise error.TestFail(
                        "The DUT didn't block a request from localhost using "
                        "the address %s." % host_ip)

        # Check we can download if the connection comes from a peer on the
        # network. To achieve this, we forward the tester's TCP traffic through
        # a fake host on lansim.
        self._sim.run_on_simulator(lambda: peer.tcp_forward(1234, ips[0], port))

        ret = self._p2p_fetch(peer.ip_addr, 1234, 'file')
        if ret != content:
            logging.error('curl returned: %s', repr(ret)[:100])
            raise error.TestFail(
                    "The DUT didn't serve the file request from %s " %
                    peer.id_addr)
