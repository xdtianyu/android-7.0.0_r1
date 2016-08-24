# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import dbus
import dbus.types
import os
import time

from autotest_lib.client.bin import test
from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.cellular import mm1_constants
from autotest_lib.client.cros.cellular import test_environment
from autotest_lib.client.cros.networking import pm_proxy

I_ACTIVATION_TEST = 'Interface.CDMAActivationTest'
ACTIVATION_STATE_TIMEOUT = 10
MODEM_STATE_TIMEOUT = 10
TEST_MODEMS_MODULE_PATH = os.path.join(os.path.dirname(__file__), 'files',
                                       'modems.py')

class ActivationTest(object):
    """
    Super class that implements setup code that is common to the individual
    tests.

    """
    def __init__(self, test):
        self.test = test
        self.modem_properties_interface = None


    def run(self):
        """
        Restarts the pseudomodem with the modem object to be used for this
        test and runs the test.

        """
        self.pseudomm = pm_proxy.PseudoMMProxy.get_proxy()
        self._run_test()


    def _set_modem_activation_state(self, state):
        self.pseudomm.get_modem().iface_properties.Set(
                mm1_constants.I_MODEM_CDMA,
                'ActivationState',
                dbus.types.UInt32(state))


    def _get_modem_activation_state(self):
        modem = self.pseudomm.get_modem()
        return modem.properties(mm1_constants.I_MODEM_CDMA)['ActivationState']


    def pseudomodem_flags(self):
        """
        Subclasses must override this method to setup the flags map passed to
        pseudomodem to suite their needs.

        """
        raise NotImplementedError()


    def _run_test(self):
        raise NotImplementedError()

class ActivationStateTest(ActivationTest):
    """
    This test verifies that the service "ActivationState" property matches the
    cdma activation state exposed by ModemManager.

    """
    def pseudomodem_flags(self):
        return {'family' : 'CDMA'}


    def _run_test(self):
        self.test.reset_modem()

        # The modem state should be REGISTERED.
        self.test.check_modem_state(mm1_constants.MM_MODEM_STATE_REGISTERED)

        # Service should appear as 'activated'.
        self.test.check_service_activation_state('activated')

        # Service activation state should change to 'not-activated'.
        self._set_modem_activation_state(
                mm1_constants.MM_MODEM_CDMA_ACTIVATION_STATE_NOT_ACTIVATED)
        self.test.check_service_activation_state('not-activated')

        # Service activation state should change to 'activating'.
        self._set_modem_activation_state(
                mm1_constants.MM_MODEM_CDMA_ACTIVATION_STATE_ACTIVATING)
        self.test.check_service_activation_state('activating')

        # Service activation state should change to 'partially-activated'.
        st = mm1_constants.MM_MODEM_CDMA_ACTIVATION_STATE_PARTIALLY_ACTIVATED
        self._set_modem_activation_state(st)
        self.test.check_service_activation_state('partially-activated')

        # Service activation state should change to 'activated'.
        self._set_modem_activation_state(
                mm1_constants.MM_MODEM_CDMA_ACTIVATION_STATE_ACTIVATED)
        self.test.check_service_activation_state('activated')


class ActivationSuccessTest(ActivationTest):
    """
    This test verifies that the service finally bacomes "activated" when the
    service is told to initiate OTASP activation.

    """
    def pseudomodem_flags(self):
        return {'test-module' : TEST_MODEMS_MODULE_PATH,
                'test-modem-class' : 'UnactivatedCdmaModem'}


    def _run_test(self):
        self.test.reset_modem()

        # The modem state should be REGISTERED.
        self.test.check_modem_state(mm1_constants.MM_MODEM_STATE_REGISTERED)

        # Service should appear as 'not-activated'.
        self.test.check_service_activation_state('not-activated')

        # Call 'CompleteActivation' on the service. The service should become
        # 'activating'.
        service = self.test.test_env.shill.find_cellular_service_object()
        service.CompleteCellularActivation()
        self.test.check_service_activation_state('activating')

        # The modem should reset in 5 seconds. Wait 5 more seconds to make sure
        # a new service gets created.
        time.sleep(10)
        self.test.check_service_activation_state('activated')


class ActivationFailureRetryTest(ActivationTest):
    """
    This test verifies that if "ActivateAutomatic" fails, a retry will be
    scheduled.

    """
    NUM_ACTIVATE_RETRIES = 5
    def pseudomodem_flags(self):
        return {'test-module' : TEST_MODEMS_MODULE_PATH,
                'test-modem-class' : 'ActivationRetryModem',
                'test-modem-arg' : [self.NUM_ACTIVATE_RETRIES]}


    def _run_test(self):
        self.test.reset_modem()

        # The modem state should be REGISTERED.
        self.test.check_modem_state(mm1_constants.MM_MODEM_STATE_REGISTERED)

        # Service should appear as 'not-activated'.
        self.test.check_service_activation_state('not-activated')

        # Call 'CompleteActivation' on the service.
        service = self.test.test_env.shill.find_cellular_service_object()
        service.CompleteCellularActivation()

        # Wait for shill to retry the failed activations, except the last retry
        # will succeed.
        # NOTE: Don't check for transitory service activation states while this
        # is happening because shill will reset the modem once the activation
        # succeeds which will cause the existing service to get deleted.
        modem = self.pseudomm.get_modem()
        utils.poll_for_condition(
                lambda: (modem.properties(I_ACTIVATION_TEST)['ActivateCount'] ==
                         self.NUM_ACTIVATE_RETRIES),
                exception=error.TestFail(
                        'Shill did not retry failed activation'),
                timeout=10)

        # The modem should reset in 5 seconds. Wait 5 more seconds to make sure
        # a new service gets created.
        time.sleep(10)
        self.test.check_service_activation_state('activated')


class network_CDMAActivate(test.test):
    """
    Tests various scenarios that may arise during the post-payment CDMA
    activation process when shill accesses the modem via ModemManager.

    """
    version = 1

    def check_modem_state(self, expected_state, timeout=MODEM_STATE_TIMEOUT):
        """
        Polls until the modem has the expected state within |timeout| seconds.

        @param expected_state: The modem state the modem is expected to be in.
        @param timeout: The timeout interval for polling.

        @raises pm_proxy.ModemManager1ProxyError if the modem doesn't
                transition to |expected_state| within |timeout|.

        """
        modem = pm_proxy.PseudoMMProxy.get_proxy().get_modem()
        modem.wait_for_states([expected_state], timeout_seconds=timeout)


    def check_service_activation_state(self, expected_state):
        """
        Waits until the current cellular service has the expected activation
        state within ACTIVATION_STATE_TIMEOUT seconds.

        @param expected_state: The activation state the service is expected to
                               be in.
        @raises error.TestFail, if no cellular service is found or the service
                activation state doesn't match |expected_state| within timeout.

        """
        success, state, _ = self.test_env.shill.wait_for_property_in(
                self.test_env.shill.find_cellular_service_object(),
                'Cellular.ActivationState',
                [expected_state],
                ACTIVATION_STATE_TIMEOUT)
        if not success:
            raise error.TestFail(
                    'Service activation state should be \'%s\', but it is '
                    '\'%s\'.' % (expected_state, state))


    def reset_modem(self):
        """
        Resets the one and only modem in the DUT.

        """
        modem = self.test_env.shill.find_cellular_device_object()
        self.test_env.shill.reset_modem(modem)


    def run_once(self):
        tests = [
            ActivationStateTest(self),
            ActivationSuccessTest(self),
            ActivationFailureRetryTest(self)
        ]

        for test in tests:
            self.test_env = test_environment.CellularPseudoMMTestEnvironment(
                    pseudomm_args=(test.pseudomodem_flags(),))
            with self.test_env:
                test.run()
