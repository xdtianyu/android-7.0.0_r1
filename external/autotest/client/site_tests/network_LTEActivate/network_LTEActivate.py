# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import dbus
import logging
import os
import time

from autotest_lib.client.bin import test
from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.cellular import mm1_constants
from autotest_lib.client.cros.cellular import test_environment
from autotest_lib.client.cros.networking import pm_proxy

I_ACTIVATION_TEST = 'Interface.LTEActivationTest'
TEST_MODEMS_MODULE_PATH = os.path.join(os.path.dirname(__file__), 'files',
                                       'modems.py')

LONG_TIMEOUT = 20
SHORT_TIMEOUT = 10

class ActivationTest(object):
    """
    Super class that implements setup code that is common to the individual
    tests.

    """
    def __init__(self, test):
        self.test = test


    def Cleanup(self):
        """
        Makes the modem look like it has been activated to satisfy the test
        end condition.

        """
        # Set the MDN to a non-zero value, so that shill removes the ICCID from
        # activating_iccid_store.profile. This way, individual test runs won't
        # interfere with each other.
        modem = self.test.pseudomm.wait_for_modem(timeout_seconds=LONG_TIMEOUT)
        modem.iface_properties.Set(mm1_constants.I_MODEM,
                                   'OwnNumbers',
                                   ['1111111111'])
        # Put the modem in the unknown subscription state so that the mdn value is
        # used to remove the iccid entry
        self.test.pseudomm.iface_testing.SetSubscriptionState(
                mm1_constants.MM_MODEM_3GPP_SUBSCRIPTION_STATE_UNKNOWN,
                mm1_constants.MM_MODEM_3GPP_SUBSCRIPTION_STATE_UNKNOWN)
        time.sleep(5)
        self.test.CheckServiceActivationState('activated')


    def Run(self):
        """
        Configures the pseudomodem to run with the test modem, runs the test
        and cleans up.

        """
        self.RunTest()
        self.Cleanup()


    def TestModemClass(self):
        """ Returns the name of the custom modem to use for this test. """
        raise NotImplementedError()


    def RunTest(self):
        """
        Runs the body of the test. Should be implemented by the subclass.

        """
        raise NotImplementedError()


class ActivationResetTest(ActivationTest):
    """
    This test verifies that the modem resets after online payment.

    """
    def TestModemClass(self):
        return 'TestModem'


    def RunTest(self):
        # Service should appear as 'not-activated'.
        self.test.CheckServiceActivationState('not-activated')
        self.test.CheckResetCalled(False)

        # Call 'CompleteActivation' on the device. The service will become
        # 'activating' and the modem should reset immediately.
        # Not checking for the intermediate 'activating' state because it makes
        # the test too fragile
        service = self.test.FindCellularService()
        service.CompleteCellularActivation()
        time.sleep(SHORT_TIMEOUT)
        self.test.CheckResetCalled(True)


class ActivationCompleteTest(ActivationTest):
    """
    This test verifies that the service eventually becomes 'activated' in the
    case of a post-payment registration and the modem finally registers
    to a network after a reset.

    """
    def TestModemClass(self):
        return 'ResetRequiredForActivationModem'


    def RunTest(self):
        # Service should appear as 'not-activated'.
        self.test.CheckServiceActivationState('not-activated')
        self.test.CheckResetCalled(False)

        # Call 'CompleteActivation' on the device. The service will become
        # 'activating' and the modem should reset immediately.
        # Not checking for the intermediate 'activating' state because it makes
        # the test too fragile
        service = self.test.FindCellularService()
        service.CompleteCellularActivation()
        time.sleep(SHORT_TIMEOUT)
        self.test.CheckResetCalled(True)

        # The service should register and be marked as 'activated'.
        self.test.CheckServiceActivationState('activated')


class ActivationDueToMdnTest(ActivationTest):
    """
    This test verifies that a valid MDN should cause the service to get marked
    as 'activated' when the modem is in unknown subscription state.

    """
    def TestModemClass(self):
        return 'TestModem'


    def RunTest(self):
        # Service should appear as 'not-activated'.
        self.test.CheckServiceActivationState('not-activated')

        # Update the MDN. The service should get marked as activated.
        modem = self.test.pseudomm.get_modem()
        modem.iface_properties.Set(mm1_constants.I_MODEM,
                                   'OwnNumbers',
                                   ['1111111111'])
        # Put the modem in the unknown subscription state so that the mdn value is
        # used to determine the service activation status.
        self.test.pseudomm.iface_testing.SetSubscriptionState(
                mm1_constants.MM_MODEM_3GPP_SUBSCRIPTION_STATE_UNKNOWN,
                mm1_constants.MM_MODEM_3GPP_SUBSCRIPTION_STATE_UNKNOWN)
        time.sleep(SHORT_TIMEOUT)
        self.test.CheckServiceActivationState('activated')


class network_LTEActivate(test.test):
    """
    After an online payment to activate a network, shill keeps track of service
    activation by monitoring changes to network registration and MDN updates
    combined with a modem reset. The test checks that the
    Cellular.ActivationState property of the service has the correct value
    associated with it by simulating possible scenarios using the pseudo modem
    manager.

    """
    version = 1

    def GetModemState(self):
        """Returns the current ModemManager modem state."""
        modem = self.pseudomm.get_modem()
        props = modem.properties(mm1_constants.I_MODEM)
        return props['State']


    def SetResetCalled(self, value):
        """
        Sets the value of the "ResetCalled" property of the current
        modem.

        @param value: Value to set in the property.

        """
        modem = self.pseudomm.get_modem()
        if modem is None:
            return
        modem.iface_properties.Set(
                I_ACTIVATION_TEST,
                'ResetCalled',
                dbus.types.Boolean(value))


    def GetResetCalled(self, modem):
        """
        Returns the current value of the "ResetCalled" property of the current
        modem.

        @param modem: Modem proxy to send the query to.

        """
        return modem.properties(I_ACTIVATION_TEST)['ResetCalled']


    def _CheckResetCalledHelper(self, expected_value):
        modem = self.pseudomm.get_modem()
        if modem is None:
            return False
        try:
            return self.GetResetCalled(modem) == expected_value
        except dbus.exceptions.DBusException as e:
            name = e.get_dbus_name()
            if (name == mm1_constants.DBUS_UNKNOWN_METHOD or
                name == mm1_constants.DBUS_UNKNOWN_OBJECT):
                return False
            raise e


    def CheckResetCalled(self, expected_value):
        """
        Checks that the ResetCalled property on the modem matches the expect
        value.

        @param expected_value: The expected value of ResetCalled.

        """
        utils.poll_for_condition(
            lambda: self._CheckResetCalledHelper(expected_value),
            exception=error.TestFail("\"ResetCalled\" did not match: " +
                                     str(expected_value)),
            timeout=LONG_TIMEOUT)


    def EnsureModemStateReached(self, expected_state, timeout):
        """
        Asserts that the underlying modem state becomes |expected_state| within
        |timeout|.

        @param expected_state: The expected modem state.
        @param timeout: Timeout in which the condition should be met.

        """
        utils.poll_for_condition(
                lambda: self.GetModemState() == expected_state,
                exception=error.TestFail(
                        'Modem failed to reach state ' +
                        mm1_constants.ModemStateToString(expected_state)),
                timeout=timeout)


    def CheckServiceActivationState(self, expected_state):
        """
        Asserts that the service activation state matches |expected_state|
        within SHORT_TIMEOUT.

        @param expected_state: The expected service activation state.

        """
        logging.info('Checking for service activation state: %s',
                     expected_state)
        service = self.FindCellularService()
        success, state, duration = self.test_env.shill.wait_for_property_in(
            service,
            'Cellular.ActivationState',
            [expected_state],
            SHORT_TIMEOUT)
        if not success and state != expected_state:
            raise error.TestError(
                'Service activation state should be \'%s\', but it is \'%s\'.'
                % (expected_state, state))


    def FindCellularService(self, check_not_none=True):
        """
        Returns the current cellular service.

        @param check_not_none: If True, an error will be raised if no service
                was found.

        """
        if check_not_none:
            utils.poll_for_condition(
                    lambda: (self.test_env.shill.find_cellular_service_object()
                             is not None),
                    exception=error.TestError(
                            'Could not find cellular service within timeout.'),
                    timeout=LONG_TIMEOUT);

        service = self.test_env.shill.find_cellular_service_object()

        # Check once more, to make sure it's valid.
        if check_not_none and not service:
            raise error.TestError('Could not find cellular service.')
        return service


    def FindCellularDevice(self):
        """Returns the current cellular device."""
        device = self.test_env.shill.find_cellular_device_object()
        if not device:
            raise error.TestError('Could not find cellular device.')
        return device


    def ResetCellularDevice(self):
        """
        Resets all modems, guaranteeing that the operation succeeds and doesn't
        fail due to race conditions in pseudomodem start-up and test execution.

        """
        self.EnsureModemStateReached(
                mm1_constants.MM_MODEM_STATE_ENABLED, SHORT_TIMEOUT)
        self.test_env.shill.reset_modem(self.FindCellularDevice())
        self.EnsureModemStateReached(
                mm1_constants.MM_MODEM_STATE_ENABLED, SHORT_TIMEOUT)


    def run_once(self):
        tests = [
            ActivationResetTest(self),
            ActivationCompleteTest(self),
            ActivationDueToMdnTest(self),
        ]

        for test in tests:
            self.test_env = test_environment.CellularPseudoMMTestEnvironment(
                    pseudomm_args = ({'family' : '3GPP',
                                      'test-module' : TEST_MODEMS_MODULE_PATH,
                                      'test-modem-class' : test.TestModemClass(),
                                      'test-sim-class' : 'TestSIM'},))
            with self.test_env:
                self.pseudomm = pm_proxy.PseudoMMProxy.get_proxy()
                # Set the reset flag to False explicitly before each test
                # sequence starts to ignore the reset as a part of the test init
                self.SetResetCalled(False)
                test.Run()
