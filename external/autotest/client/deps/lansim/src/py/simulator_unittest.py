# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import threading
import select
import socket
import subprocess
import sys
import unittest

from lansim import host
from lansim import simulator
from lansim import tuntap


def raise_exception():
    """Raises an exception."""
    raise Exception('Something bad.')


class InfoTCPServer(threading.Thread):
    """A TCP server running on a separated thread.

    This simple TCP server thread listen for connections for every new
    connection it sends the address information of the connected client.
    """
    def __init__(self, host, port):
        """Creates the TCP server on the host:port address.

        @param host: The IP address in plain text.
        @param port: The TCP port number where the server listens on."""
        threading.Thread.__init__(self)
        self._port = port
        self._sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self._sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self._sock.bind((host, port))
        self._sock.listen(1)
        self._must_exit = False


    def run(self):
        while not self._must_exit:
            # Check the must_exit flag every second.
            rlist, wlist, xlist = select.select([self._sock], [], [], 1.)
            if self._sock in rlist:
                conn, (addr, port) = self._sock.accept()
                # Send back the client address, port and our port
                conn.send('%s %d %d' % (addr, port,  self._port))
                conn.close()
        self._sock.close()


    def stop(self):
        """Signal the termination of the running thread."""
        self._must_exit = True


def GetInfoTCP(host, port):
    """Connects to a InfoTCPServer on host:port and reads all the information.

    @param host: The host where the InfoTCPServer is running.
    @param port: The port where the InfoTCPServer is running.
    """
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect((host, port))
    data = sock.recv(1024)
    sock.close()
    return data


class SimulatorTest(unittest.TestCase):
    """Unit tests for the Simulator class."""

    def setUp(self):
        """Creates a Simulator under test over a TAP device."""
        self._tap = tuntap.TunTap(tuntap.IFF_TAP, name="faketap")
        # According to RFC 3927 (Dynamic Configuration of IPv4 Link-Local
        # Addresses), a host can pseudorandomly assign an IPv4 address on the
        # 169.254/16 network to communicate with other devices on the same link
        # on absence of a DHCP server and other source of network configuration.
        # The tests on this class explicitly specify the interface to use, so
        # they can run in parallel even when there are more than one interface
        # with the same IPv4 address. A TUN/TAP interface with an IPv4 address
        # on this range shouldn't collide with any useful service running on a
        # different (physical) interface.
        self._tap.set_addr('169.254.11.11')
        self._tap.up()

        self._sim = simulator.Simulator(self._tap)


    def tearDown(self):
        """Stops and destroy the interface."""
        self._tap.down()


    def testTimeout(self):
        """Tests that the Simulator can start and run for a short time."""
        # Run for at most 100ms and finish the test. This implies that the
        # stop() method works.
        self._sim.run(timeout=0.1)


    def testRemoveTimeout(self):
        """Tests that the Simulator can remove unfired timeout calls."""
        # Schedule the callback far in time, run the simulator for a short time
        # and remove it.
        self._sim.add_timeout(60, raise_exception)
        self._sim.run(timeout=0.1)
        self.assertTrue(self._sim.remove_timeout(raise_exception))
        self.assertFalse(self._sim.remove_timeout(raise_exception))


    def testUntil(self):
        """Tests that the Simulator can start run until a condition is met."""
        tasks_done = []
        # After 0.2 seconds we add a task to tasks_done that should break the
        # loop. If it doesn't, the a second value will be added making the test
        # fail.
        self._sim.add_timeout(0.2, lambda: tasks_done.append('good task'))
        self._sim.add_timeout(4.0, lambda: tasks_done.append('bad task'))
        self._sim.run(timeout=5.0, until=lambda: tasks_done)
        self.assertEqual(len(tasks_done), 1)


    def testHost(self):
        """Tests that the Simulator can add rules from the SimpleHost."""
        # The IP and MAC addresses simulated are unknown to the rest of the
        # system as they only live on this interface. Again, any IP on the
        # network 169.254/16 should not cause any problem with other services
        # running on this host.
        host.SimpleHost(self._sim, '12:34:56:78:90:AB', '169.254.11.22')
        self._sim.run(timeout=0.1)


class SimulatorThreadTest(unittest.TestCase):
    """Unit tests for the SimulatorThread class."""

    def setUp(self):
        """Creates a SimulatorThread under test over a TAP device."""
        self._tap = tuntap.TunTap(tuntap.IFF_TAP, name="faketap")
        # See note about IP addresses on SimulatorTest.setUp().
        self._ip_addr = '169.254.11.11'
        self._tap.set_addr(self._ip_addr)
        self._tap.up()

        # 20 seconds timeout for unittest completion (they should run in about
        # 2 seconds each).
        self._sim = simulator.SimulatorThread(self._tap, timeout=20)


    def tearDown(self):
        """Stops and destroy the thread."""
        self._sim.stop() # stop() is idempotent.
        self._sim.join()
        self._tap.down()
        if self._sim.error:
            sys.stderr.write('SimulatorThread exception: %r' % self._sim.error)
            sys.stderr.write(self._sim.traceback)
            raise self._sim.error


    def testError(self):
        """Exceptions raised on the thread appear on the exc_info member."""
        self._sim.add_timeout(0.1, raise_exception)
        self._sim.start()
        self._sim.join()
        self.assertEqual(self._sim.error.message, 'Something bad.')
        # Clean the error before tearDown()
        self._sim.error = None


    def testARPPing(self):
        """Test that the simulator properly handles a ARP request/response."""
        host.SimpleHost(self._sim, '12:34:56:78:90:22', '169.254.11.22')
        host.SimpleHost(self._sim, '12:34:56:78:90:33', '169.254.11.33')
        host.SimpleHost(self._sim, '12:34:56:78:90:44', '169.254.11.33')

        self._sim.start()
        # arping and wait for one second for the responses.
        out = subprocess.check_output(
                ['arping', '-I', self._tap.name, '169.254.11.22',
                 '-c', '1', '-w', '1'])
        resp = [line for line in out.splitlines() if 'Unicast reply' in line]
        self.assertEqual(len(resp), 1)
        self.assertTrue(resp[0].startswith(
                'Unicast reply from 169.254.11.22 [12:34:56:78:90:22]'))

        out = subprocess.check_output(
                ['arping', '-I', self._tap.name, '169.254.11.33',
                 '-c', '1', '-w', '1'])
        resp = [line for line in out.splitlines() if 'Unicast reply' in line]
        self.assertEqual(len(resp), 2)
        resp.sort()
        self.assertTrue(resp[0].startswith(
                'Unicast reply from 169.254.11.33 [12:34:56:78:90:33]'))
        self.assertTrue(resp[1].startswith(
                'Unicast reply from 169.254.11.33 [12:34:56:78:90:44]'))


    def testTCPForward(self):
        """Host can forward TCP traffic back to the kernel network stack."""
        h = host.SimpleHost(self._sim, '12:34:56:78:90:22', '169.254.11.22')
        # Launch two TCP servers on the network interface end.
        srv1 = InfoTCPServer(self._ip_addr, 1080)
        srv1.start()
        srv2 = InfoTCPServer(self._ip_addr, 1081)
        srv2.start()

        # Map those two ports to a given IP address on the fake network.
        h.tcp_forward(80, self._ip_addr, 1080)
        h.tcp_forward(81, self._ip_addr, 1081)

        # Start the simulation.
        self._sim.start()

        try:
            srv1data = GetInfoTCP('169.254.11.22', 80)
            srv2data = GetInfoTCP('169.254.11.22', 81)
        finally:
            srv1.stop()
            srv2.stop()
            srv1.join()
            srv2.join()

        # First connection is seen from the .11.22:1024 client.
        self.assertEqual(srv1data, '169.254.11.22 1024 1080')
        # Second connection is seen from the .11.22:1024 client because is made
        # to a different port.
        self.assertEqual(srv2data, '169.254.11.22 1024 1081')


    def testWaitForCondition(self):
        """Main thread can wait until a condition is met on the simulator."""
        self._sim.start()

        # Wait for an always False condition.
        condition = lambda: False
        ret = self._sim.wait_for_condition(condition, timeout=1.5)
        self.assertFalse(ret)

        # Wait for a trivially True condition.
        condition = lambda: True
        ret = self._sim.wait_for_condition(condition, timeout=10.)
        self.assertTrue(ret)

        # Without timeout.
        ret = self._sim.wait_for_condition(condition, timeout=None)
        self.assertTrue(ret)

        # Wait for a condition that takes 3 calls to meet.
        var = []
        condition = lambda: var if len(var) == 3 else var.append(None)
        ret = self._sim.wait_for_condition(condition, timeout=10.)
        self.assertEqual(len(ret), 3)

if __name__ == '__main__':
    unittest.main()
