# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
Programmable testing DHCP server.

Simple DHCP server you can program with expectations of future packets and
responses to those packets.  The server is basically a thin wrapper around a
server socket with some utility logic to make setting up tests easier.  To write
a test, you start a server, construct a sequence of handling rules.

Handling rules let you set up expectations of future packets of certain types.
Handling rules are processed in order, and only the first remaining handler
handles a given packet.  In theory you could write the entire test into a single
handling rule and keep an internal state machine for how far that handler has
gotten through the test.  This would be poor style however.  Correct style is to
write (or reuse) a handler for each packet the server should see, leading us to
a happy land where any conceivable packet handler has already been written for
us.

Example usage:

# Start up the DHCP server, which will ignore packets until a test is started
server = DhcpTestServer(interface="veth_master")
server.start()

# Given a list of handling rules, start a test with a 30 sec timeout.
handling_rules = []
handling_rules.append(DhcpHandlingRule_RespondToDiscovery(intended_ip,
                                                          intended_subnet_mask,
                                                          dhcp_server_ip,
                                                          lease_time_seconds)
server.start_test(handling_rules, 30.0)

# Trigger DHCP clients to do various test related actions
...

# Get results
server.wait_for_test_to_finish()
if (server.last_test_passed):
    ...
else:
    ...


Note that if you make changes, make sure that the tests in dhcp_unittest.py
still pass.
"""

import logging
import socket
import threading
import time
import traceback

from autotest_lib.client.cros import dhcp_packet
from autotest_lib.client.cros import dhcp_handling_rule

# From socket.h
SO_BINDTODEVICE = 25

class DhcpTestServer(threading.Thread):
    def __init__(self,
                 interface=None,
                 ingress_address="<broadcast>",
                 ingress_port=67,
                 broadcast_address="255.255.255.255",
                 broadcast_port=68):
        super(DhcpTestServer, self).__init__()
        self._mutex = threading.Lock()
        self._ingress_address = ingress_address
        self._ingress_port = ingress_port
        self._broadcast_port = broadcast_port
        self._broadcast_address = broadcast_address
        self._socket = None
        self._interface = interface
        self._stopped = False
        self._test_in_progress = False
        self._last_test_passed = False
        self._test_timeout = 0
        self._handling_rules = []
        self._logger = logging.getLogger("dhcp.test_server")
        self._exception = None
        self.daemon = False

    @property
    def stopped(self):
        with self._mutex:
            return self._stopped

    @property
    def is_healthy(self):
        with self._mutex:
            return self._socket is not None

    @property
    def test_in_progress(self):
        with self._mutex:
            return self._test_in_progress

    @property
    def last_test_passed(self):
        with self._mutex:
            return self._last_test_passed

    @property
    def current_rule(self):
        """
        Return the currently active DhcpHandlingRule.
        """
        with self._mutex:
            return self._handling_rules[0]

    def start(self):
        """
        Start the DHCP server.  Only call this once.
        """
        if self.is_alive():
            return False
        self._logger.info("DhcpTestServer started; opening sockets.")
        try:
            self._socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            self._logger.info("Opening socket on '%s' port %d." %
                              (self._ingress_address, self._ingress_port))
            self._socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            self._socket.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
            if self._interface is not None:
                self._logger.info("Binding to %s" % self._interface)
                self._socket.setsockopt(socket.SOL_SOCKET,
                                        SO_BINDTODEVICE,
                                        self._interface)
            self._socket.bind((self._ingress_address, self._ingress_port))
            # Wait 100 ms for a packet, then return, thus keeping the thread
            # active but mostly idle.
            self._socket.settimeout(0.1)
        except socket.error, socket_error:
            self._logger.error("Socket error: %s." % str(socket_error))
            self._logger.error(traceback.format_exc())
            if not self._socket is None:
                self._socket.close()
            self._socket = None
            self._logger.error("Failed to open server socket.  Aborting.")
            return
        super(DhcpTestServer, self).start()

    def stop(self):
        """
        Stop the DHCP server and free its socket.
        """
        with self._mutex:
            self._stopped = True

    def start_test(self, handling_rules, test_timeout_seconds):
        """
        Start a new test using |handling_rules|.  The server will call the
        test successfull if it receives a RESPONSE_IGNORE_SUCCESS (or
        RESPONSE_RESPOND_SUCCESS) from a handling_rule before
        |test_timeout_seconds| passes.  If the timeout passes without that
        message, the server runs out of handling rules, or a handling rule
        return RESPONSE_FAIL, the test is ended and marked as not passed.

        All packets received before start_test() is called are received and
        ignored.
        """
        with self._mutex:
            self._test_timeout = time.time() + test_timeout_seconds
            self._handling_rules = handling_rules
            self._test_in_progress = True
            self._last_test_passed = False
            self._exception = None

    def wait_for_test_to_finish(self):
        """
        Block on the test finishing in a CPU friendly way.  Timeouts, successes,
        and failures count as finishes.
        """
        while self.test_in_progress:
            time.sleep(0.1)
        if self._exception:
            raise self._exception

    def abort_test(self):
        """
        Abort a test prematurely, counting the test as a failure.
        """
        with self._mutex:
            self._logger.info("Manually aborting test.")
            self._end_test_unsafe(False)

    def _teardown(self):
        with self._mutex:
            self._socket.close()
            self._socket = None

    def _end_test_unsafe(self, passed):
        if not self._test_in_progress:
            return
        if passed:
            self._logger.info("DHCP server says test passed.")
        else:
            self._logger.info("DHCP server says test failed.")
        self._test_in_progress = False
        self._last_test_passed = passed

    def _send_response_unsafe(self, packet):
        if packet is None:
            self._logger.error("Handling rule failed to return a packet.")
            return False
        self._logger.debug("Sending response: %s" % packet)
        binary_string = packet.to_binary_string()
        if binary_string is None or len(binary_string) < 1:
            self._logger.error("Packet failed to serialize to binary string.")
            return False

        self._socket.sendto(binary_string,
                            (self._broadcast_address, self._broadcast_port))
        return True

    def _loop_body(self):
        with self._mutex:
            if self._test_in_progress and self._test_timeout < time.time():
                # The test has timed out, so we abort it.  However, we should
                # continue to accept packets, so we fall through.
                self._logger.error("Test in progress has timed out.")
                self._end_test_unsafe(False)
            try:
                data, _ = self._socket.recvfrom(1024)
                self._logger.info("Server received packet of length %d." %
                                   len(data))
            except socket.timeout:
                # No packets available, lets return and see if the server has
                # been shut down in the meantime.
                return

            # Receive packets when no test is in progress, just don't process
            # them.
            if not self._test_in_progress:
                return

            packet = dhcp_packet.DhcpPacket(byte_str=data)
            if not packet.is_valid:
                self._logger.warning("Server received an invalid packet over a "
                                     "DHCP port?")
                return

            logging.debug("Server received a DHCP packet: %s." % packet)
            if len(self._handling_rules) < 1:
                self._logger.info("No handling rule for packet: %s." %
                                  str(packet))
                self._end_test_unsafe(False)
                return

            handling_rule = self._handling_rules[0]
            response_code = handling_rule.handle(packet)
            logging.info("Handler gave response: %d" % response_code)
            if response_code & dhcp_handling_rule.RESPONSE_POP_HANDLER:
                self._handling_rules.pop(0)

            if response_code & dhcp_handling_rule.RESPONSE_HAVE_RESPONSE:
                for response_instance in range(
                        handling_rule.response_packet_count):
                    response = handling_rule.respond(packet)
                    if not self._send_response_unsafe(response):
                        self._logger.error(
                                "Failed to send packet, ending test.")
                        self._end_test_unsafe(False)
                        return

            if response_code & dhcp_handling_rule.RESPONSE_TEST_FAILED:
                self._logger.info("Handling rule %s rejected packet %s." %
                                  (handling_rule, packet))
                self._end_test_unsafe(False)
                return

            if response_code & dhcp_handling_rule.RESPONSE_TEST_SUCCEEDED:
                self._end_test_unsafe(True)
                return

    def run(self):
        """
        Main method of the thread.  Never call this directly, since it assumes
        some setup done in start().
        """
        with self._mutex:
            if self._socket is None:
                self._logger.error("Failed to create server socket, exiting.")
                return

        self._logger.info("DhcpTestServer entering handling loop.")
        while not self.stopped:
            try:
                self._loop_body()
                # Python does not have waiting queues on Lock objects.
                # Give other threads a change to hold the mutex by
                # forcibly releasing the GIL while we sleep.
                time.sleep(0.01)
            except Exception as e:
                with self._mutex:
                    self._end_test_unsafe(False)
                    self._exception = e
        with self._mutex:
            self._end_test_unsafe(False)
        self._logger.info("DhcpTestServer closing sockets.")
        self._teardown()
        self._logger.info("DhcpTestServer exiting.")
