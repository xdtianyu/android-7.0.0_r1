#!/usr/bin/python
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import mox
import unittest

from config import rpm_config
import rpm_dispatcher

DUT_SAME_RPM1 = 'chromeos-rack8e-hostbs1'
DUT_SAME_RPM2 = 'chromeos-rack8e-hostbs2'
RPM_HOSTNAME = 'chromeos-rack8e-rpm1'
DUT_DIFFERENT_RPM = 'chromeos-rack1-hostbs1'
FAKE_DISPATCHER_URI = 'fake-dispatcher'
FAKE_DISPATCHER_PORT = 9999
FRONT_END_URI = rpm_config.get('RPM_INFRASTRUCTURE', 'frontend_uri')
PROPER_URI_FORMAT = 'http://%s:%d'


class TestRPMDispatcher(mox.MoxTestBase):
    """
    Simple unit tests to verify that the RPM Dispatcher properly registers with
    the frontend server, and also initializes and reuses the same RPM Controller
    for DUT requests on the same RPM.

    queue_request is the only public method of RPM Dispatcher, however its logic
    is simple and relies mostly on the private methods; therefore, I am testing
    primarily RPMDispatcher initialization and _get_rpm_controller (which calls
    _create_rpm_controller) to verify correct implementation.
    """

    def setUp(self):
        super(TestRPMDispatcher, self).setUp()
        self.frontend_mock = self.mox.CreateMockAnything()
        expected_uri = PROPER_URI_FORMAT % (FAKE_DISPATCHER_URI,
                                            FAKE_DISPATCHER_PORT)
        self.frontend_mock.register_dispatcher(expected_uri)
        rpm_dispatcher.xmlrpclib.ServerProxy = self.mox.CreateMockAnything()
        rpm_dispatcher.xmlrpclib.ServerProxy(FRONT_END_URI).AndReturn(
                self.frontend_mock)
        rpm_dispatcher.atexit = self.mox.CreateMockAnything()
        rpm_dispatcher.atexit.register(mox.IgnoreArg())
        self.mox.ReplayAll()
        self.dispatcher = rpm_dispatcher.RPMDispatcher(FAKE_DISPATCHER_URI,
                                                       FAKE_DISPATCHER_PORT)


    def testRegistration(self):
        """
        Make sure that as a dispatcher is initialized it properly registered
        with the frontend server.
        """
        self.mox.VerifyAll()


    def testGetSameRPMController(self):
        """
        Make sure that calls to _get_rpm_controller with DUT hostnames that
        belong to the same RPM device create and retrieve the same RPMController
        instance.
        """
        controller1 = self.dispatcher._get_rpm_controller(RPM_HOSTNAME)
        controller2 = self.dispatcher._get_rpm_controller(RPM_HOSTNAME)
        self.assertEquals(controller1, controller2)


    def testGetDifferentRPMController(self):
        """
        Make sure that calls to _get_rpm_controller with DUT hostnames that
        belong to the different RPM device create and retrieve different
        RPMController instances.
        """
        controller1 = self.dispatcher._get_rpm_controller(DUT_SAME_RPM1)
        controller2 = self.dispatcher._get_rpm_controller(DUT_DIFFERENT_RPM)
        self.assertNotEquals(controller1, controller2)


if __name__ == '__main__':
    unittest.main()
