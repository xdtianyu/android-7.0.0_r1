#!/usr/bin/python
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import mox
import socket
import unittest

import frontend_server
from rpm_infrastructure_exception import RPMInfrastructureException

import common
from autotest_lib.site_utils.rpm_control_system import utils


FAKE_DISPATCHER_URI1 = 'http://fake_dispatcher:1'
FAKE_DISPATCHER_URI2 = 'http://fake_dispatcher:2'
UNREACHABLE_SERVER_MSG = 'Server Unreachable'
DUT_HOSTNAME = 'chromeos-rack8e-hostbs1'
POWERUNIT_HOSTNAME = 'chromeos-rack8e-rpm1'
OUTLET = '.A100'
NEW_STATE = 'ON'
FAKE_ERRNO = 1


class TestFrontendServer(mox.MoxTestBase):
    """Test frontend_server."""


    def setUp(self):
        super(TestFrontendServer, self).setUp()
        self.frontend = frontend_server.RPMFrontendServer()
        self.frontend._rpm_info[DUT_HOSTNAME] = utils.PowerUnitInfo(
                device_hostname=DUT_HOSTNAME,
                powerunit_type=utils.PowerUnitInfo.POWERUNIT_TYPES.RPM,
                powerunit_hostname=POWERUNIT_HOSTNAME,
                outlet=OUTLET, hydra_hostname=None)
        self.xmlrpc_mock = self.mox.CreateMockAnything()
        frontend_server.xmlrpclib.ServerProxy = self.mox.CreateMockAnything()
        frontend_server.xmlrpclib.ServerProxy(FAKE_DISPATCHER_URI1,
                allow_none=True).AndReturn(self.xmlrpc_mock)


    def testNoRegisteredDispatchers(self):
        """ Queue request with no dispatchers. Should fail. """
        self.mox.ResetAll()
        self.assertRaises(RPMInfrastructureException,
                          self.frontend.queue_request, DUT_HOSTNAME, NEW_STATE)


    def testSuccessfullyQueueRequest(self):
        """
        Queue request with at least one dispatcher.

        Expects the request to succeed.
        """
        self.xmlrpc_mock.queue_request(
                self.frontend._rpm_info[DUT_HOSTNAME],
                NEW_STATE).AndReturn(True)
        self.mox.ReplayAll()
        self.frontend.register_dispatcher(FAKE_DISPATCHER_URI1)
        self.assertTrue(self.frontend.queue_request(DUT_HOSTNAME, NEW_STATE))
        self.mox.VerifyAll()


    def testFailedQueueRequest(self):
        """
        Queue request with at least one dispatcher.

        Expects the request to fail.
        """
        self.xmlrpc_mock.queue_request(
                self.frontend._rpm_info[DUT_HOSTNAME],
                NEW_STATE).AndReturn(False)
        self.mox.ReplayAll()
        self.frontend.register_dispatcher(FAKE_DISPATCHER_URI1)
        self.assertFalse(self.frontend.queue_request(DUT_HOSTNAME, NEW_STATE))
        self.mox.VerifyAll()


    def testAllDispatchersUnregistered(self):
        """
        Queue request before and after a dispatcher unregisters.

        queue_request should return True then False.
        """
        self.xmlrpc_mock.queue_request(
                self.frontend._rpm_info[DUT_HOSTNAME],
                NEW_STATE).AndReturn(True)
        self.mox.ReplayAll()
        self.frontend.register_dispatcher(FAKE_DISPATCHER_URI1)
        self.assertTrue(self.frontend.queue_request(DUT_HOSTNAME, NEW_STATE))
        self.frontend.unregister_dispatcher(FAKE_DISPATCHER_URI1)
        self.assertRaises(RPMInfrastructureException,
                          self.frontend.queue_request, DUT_HOSTNAME, NEW_STATE)
        self.mox.VerifyAll()


    def testUnreachableDispatcherServer(self):
        """
        Make sure that if the dispatch server is unreachable and raises an error
        that we retry the call which will fail because there is no other servers
        available.

        The call to queue_request will raise a socket.error, and then it should
        return False as there is no other dispatch servers available.
        """
        self.xmlrpc_mock.queue_request(
                self.frontend._rpm_info[DUT_HOSTNAME], NEW_STATE).AndRaise(
                socket.error(FAKE_ERRNO, UNREACHABLE_SERVER_MSG))
        frontend_server.xmlrpclib.ServerProxy(
                FAKE_DISPATCHER_URI1,
                allow_none=True).AndReturn(
                self.xmlrpc_mock)
        self.xmlrpc_mock.is_up().AndRaise(
                socket.error(FAKE_ERRNO, UNREACHABLE_SERVER_MSG))
        self.mox.ReplayAll()
        self.frontend.register_dispatcher(FAKE_DISPATCHER_URI1)
        self.assertRaises(RPMInfrastructureException,
                          self.frontend.queue_request, DUT_HOSTNAME, NEW_STATE)
        self.mox.VerifyAll()


    def testUnreachableDispatcherServerWithBackup(self):
        """
        Make sure that if the dispatch server is unreachable and raises an error
        that we retry the call with a different dispatch server (if it's
        available).

        The first call to queue_request will raise a socket.error, however it
        should make a second attempt that should be successful.
        """
        self.xmlrpc_mock.queue_request(
                self.frontend._rpm_info[DUT_HOSTNAME], NEW_STATE).AndRaise(
                socket.error(FAKE_ERRNO,UNREACHABLE_SERVER_MSG))
        frontend_server.xmlrpclib.ServerProxy(
                mox.IgnoreArg(), allow_none=True).MultipleTimes(3).AndReturn(
                        self.xmlrpc_mock)
        self.xmlrpc_mock.is_up().AndRaise(
                socket.error(FAKE_ERRNO, UNREACHABLE_SERVER_MSG))
        self.xmlrpc_mock.is_up().AndReturn(True)
        self.xmlrpc_mock.queue_request(
                self.frontend._rpm_info[DUT_HOSTNAME], NEW_STATE).AndReturn(True)
        self.mox.ReplayAll()
        self.frontend.register_dispatcher(FAKE_DISPATCHER_URI1)
        self.frontend.register_dispatcher(FAKE_DISPATCHER_URI2)
        self.assertTrue(self.frontend.queue_request(DUT_HOSTNAME, NEW_STATE))
        self.mox.VerifyAll()


if __name__ == '__main__':
    unittest.main()
