# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import dbus
import logging
import time

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error

from autotest_lib.client.cros.cellular import mm1_constants
from autotest_lib.client.cros.cellular import test_environment
from autotest_lib.client.cros.networking import pm_proxy

class cellular_DeferredRegistration(test.test):
    """
    Tests that shill can handle temporary registration loss without
    disconnecting the service because some modems periodically go searching for
    a better signal while still connected to the network.  Conversely, make
    sure that shill still disconnects a service that has suffered a
    registration loss for an extended period of time (>15s).

    """
    version = 1

    DEFERRED_REGISTRATION_TIMEOUT_SECONDS = 15

    def _init(self):
        self.pseudomm = pm_proxy.PseudoMMProxy.get_proxy()
        service = self.test_env.shill.find_cellular_service_object()
        self.test_env.shill.connect_service_synchronous(
                service,
                timeout_seconds=self.test_env.shill.SERVICE_CONNECT_TIMEOUT)


    def _set_modem_registration_state(self, state):
        self.pseudomm.get_modem().iface_properties.Set(
                mm1_constants.I_MODEM_3GPP,
                'RegistrationState',
                dbus.types.UInt32(state))


    def _test_temporary_registration_loss(self):
        logging.info('Verifying temporary loss of registration behavior')
        self._set_modem_registration_state(
                mm1_constants.MM_MODEM_3GPP_REGISTRATION_STATE_SEARCHING)
        time.sleep(self.DEFERRED_REGISTRATION_TIMEOUT_SECONDS / 2)
        self._set_modem_registration_state(
                mm1_constants.MM_MODEM_3GPP_REGISTRATION_STATE_HOME)
        time.sleep(self.DEFERRED_REGISTRATION_TIMEOUT_SECONDS * 2)
        if self.test_env.shill.find_cellular_service_object() is None:
            raise error.TestFail('Cellular service should not have been '
                                 'destroyed after temporary registration loss.')
        logging.info('Successfully verified temporary loss of registration '
                     'behavior')


    def _test_permanent_registration_loss(self):
        logging.info('Verifying permanent loss of registration behavior')
        self._set_modem_registration_state(
                mm1_constants.MM_MODEM_3GPP_REGISTRATION_STATE_SEARCHING)
        time.sleep(self.DEFERRED_REGISTRATION_TIMEOUT_SECONDS * 2)
        if self.test_env.shill.find_cellular_service_object() is not None:
            raise error.TestFail('Cellular service should have been destroyed '
                                 'after permanent registration loss.')
        logging.info('Successfully verified permanent loss of registration '
                     'behavior')


    def run_once(self):
        """Called by autotest to run this test."""

        with test_environment.CellularPseudoMMTestEnvironment(
                pseudomm_args=({'family': '3GPP'},)) as self.test_env:
            self._init()

            tests = [self._test_temporary_registration_loss,
                     self._test_permanent_registration_loss]

            for test in tests:
                test()
