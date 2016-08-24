# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import time

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.cellular import mm1_constants
from autotest_lib.client.cros.cellular import test_environment
from autotest_lib.client.cros.cellular.pseudomodem import modem_3gpp
from autotest_lib.client.cros.cellular.pseudomodem import modem_cdma
from autotest_lib.client.cros.cellular.pseudomodem import pm_errors
from autotest_lib.client.cros.cellular.pseudomodem import utils as pm_utils

# Use our own connect/disconnect timeout for this test because we are using a
# a pseudomodem which should run faster than a real modem.
CONNECT_DISCONNECT_TIMEOUT = 10


def _GetModemSuperClass(family):
    """
    Obtains the correct Modem base class to use for the given family.

    @param family: The modem family. Should be one of |3GPP|/|CDMA|.
    @returns: The relevant Modem base class.
    @raises error.TestError, if |family| is not one of '3GPP' or 'CDMA'.

    """
    if family == '3GPP':
        return modem_3gpp.Modem3gpp
    elif family == 'CDMA':
        return modem_cdma.ModemCdma
    else:
        raise error.TestError('Invalid pseudomodem family: %s', family)


def GetModemDisconnectWhileStateIsDisconnecting(family):
    """
    Returns a modem that fails on disconnect request.

    @param family: The family of the modem returned.
    @returns: A modem of the given family that fails disconnect.

    """
    modem_class = _GetModemSuperClass(family)
    class _TestModem(modem_class):
        """ Actual modem implementation. """
        @pm_utils.log_dbus_method(return_cb_arg='return_cb',
                                  raise_cb_arg='raise_cb')
        def Disconnect(
            self, bearer_path, return_cb, raise_cb, *return_cb_args):
            """
            Test implementation of
            org.freedesktop.ModemManager1.Modem.Simple.Disconnect. Sets the
            modem state to DISCONNECTING and then fails, fooling shill into
            thinking that the disconnect failed while disconnecting.

            Refer to modem_simple.ModemSimple.Connect for documentation.

            """
            logging.info('Simulating failed Disconnect')
            self.ChangeState(mm1_constants.MM_MODEM_STATE_DISCONNECTING,
                             mm1_constants.MM_MODEM_STATE_CHANGE_REASON_UNKNOWN)
            time.sleep(5)
            raise pm_errors.MMCoreError(pm_errors.MMCoreError.FAILED)

    return _TestModem()


def GetModemDisconnectWhileDisconnectInProgress(family):
    """
    Returns a modem implementation that fails disconnect except the first one.

    @param family: The family of the returned modem.
    @returns: A modem of the given family that fails all but the first
            disconnect attempts.

    """
    modem_class = _GetModemSuperClass(family)
    class _TestModem(modem_class):
        """ The actual modem implementation. """
        def __init__(self):
            modem_class.__init__(self)
            self.disconnect_count = 0

        @pm_utils.log_dbus_method(return_cb_arg='return_cb',
                                  raise_cb_arg='raise_cb')
        def Disconnect(
            self, bearer_path, return_cb, raise_cb, *return_cb_args):
            """
            Test implementation of
            org.freedesktop.ModemManager1.Modem.Simple.Disconnect. Keeps
            count of successive disconnect operations and fails during all
            but the first one.

            Refer to modem_simple.ModemSimple.Connect for documentation.

            """
            # On the first call, set the state to DISCONNECTING.
            self.disconnect_count += 1
            if self.disconnect_count == 1:
                self.ChangeState(
                        mm1_constants.MM_MODEM_STATE_DISCONNECTING,
                        mm1_constants.MM_MODEM_STATE_CHANGE_REASON_UNKNOWN)
                time.sleep(5)
            else:
                raise pm_errors.MMCoreError(pm_errors.MMCoreError.FAILED)

    return _TestModem()


def GetModemDisconnectFailOther(family):
    """
    Returns a modem that fails a disconnect attempt with a generic error.

    @param family: The family of the modem returned.
    @returns: A modem of the give family that fails disconnect.

    """
    modem_class = _GetModemSuperClass(family)
    class _TestModem(modem_class):
        """ The actual modem implementation. """
        @pm_utils.log_dbus_method(return_cb_arg='return_cb',
                                  raise_cb_arg='raise_cb')
        def Disconnect(
            self, bearer_path, return_cb, raise_cb, *return_cb_args):
            """
            Test implementation of
            org.freedesktop.ModemManager1.Modem.Simple.Disconnect.
            Fails with an error.

            Refer to modem_simple.ModemSimple.Connect for documentation.

            """
            raise pm_errors.MMCoreError(pm_errors.MMCoreError.FAILED)

    return _TestModem()


class DisconnectFailTest(object):
    """
    DisconnectFailTest implements common functionality in all test cases.

    """
    def __init__(self, test, pseudomodem_family):
        self.test = test
        self._pseudomodem_family = pseudomodem_family


    def IsServiceConnected(self):
        """
        @return True, if service is connected.

        """
        service = self.test_env.shill.find_cellular_service_object()
        properties = service.GetProperties(utf8_strings=True)
        state = properties.get('State', None)
        return state in ['portal', 'online']


    def IsServiceDisconnected(self):
        """
        @return True, if service is disconnected.

        """
        service = self.test_env.shill.find_cellular_service_object()
        properties = service.GetProperties(utf8_strings=True)
        state = properties.get('State', None)
        return state == 'idle'


    def Run(self):
        """
        Runs the test.

        @raises test.TestFail, if |test_modem| hasn't been initialized.

        """
        self.test_env = test_environment.CellularPseudoMMTestEnvironment(
                pseudomm_args=(
                        {'test-module' : __file__,
                         'test-modem-class' : self._GetTestModemFunctorName(),
                         'test-modem-arg' : [self._pseudomodem_family]},))
        with self.test_env:
            self._RunTest()


    def _GetTestModemFunctorName(self):
        """ Returns the modem to be used by the pseudomodem for this test. """
        raise NotImplementedError()


    def _RunTest(self):
        raise NotImplementedError()


class DisconnectWhileStateIsDisconnectingTest(DisconnectFailTest):
    """
    Simulates a disconnect failure while the modem is still disconnecting.
    Fails if the service doesn't remain connected.

    """
    def _GetTestModemFunctorName(self):
        return 'GetModemDisconnectWhileStateIsDisconnecting'


    def _RunTest(self):
        # Connect to the service.
        service = self.test_env.shill.find_cellular_service_object()
        self.test_env.shill.connect_service_synchronous(
                service, CONNECT_DISCONNECT_TIMEOUT)

        # Disconnect attempt should fail.
        self.test_env.shill.disconnect_service_synchronous(
                service, CONNECT_DISCONNECT_TIMEOUT)

        # Service should remain connected.
        if not self.IsServiceConnected():
            raise error.TestError('Service should remain connected after '
                                  'disconnect failure.')


class DisconnectWhileDisconnectInProgressTest(DisconnectFailTest):
    """
    Simulates a disconnect failure on successive disconnects. Fails if the
    service doesn't remain connected.

    """
    def _GetTestModemFunctorName(self):
        return 'GetModemDisconnectWhileDisconnectInProgress'


    def _RunTest(self):
        # Connect to the service.
        service = self.test_env.shill.find_cellular_service_object()
        self.test_env.shill.connect_service_synchronous(
                service, CONNECT_DISCONNECT_TIMEOUT)

        # Issue first disconnect. Service should remain connected.
        self.test_env.shill.disconnect_service_synchronous(
                service, CONNECT_DISCONNECT_TIMEOUT)
        if not self.IsServiceConnected():
            raise error.TestError('Service should remain connected after '
                                  'first disconnect.')

        # Modem state should be disconnecting.
        props = self.test_env.modem.GetAll(mm1_constants.I_MODEM)
        if not props['State'] == mm1_constants.MM_MODEM_STATE_DISCONNECTING:
            raise error.TestError('Modem should be in the DISCONNECTING state.')

        # Issue second disconnect. Service should remain connected.
        self.test_env.shill.disconnect_service_synchronous(
                service, CONNECT_DISCONNECT_TIMEOUT)
        if not self.IsServiceConnected():
            raise error.TestError('Service should remain connected after '
                                  'disconnect failure.')


class DisconnectFailOtherTest(DisconnectFailTest):
    """
    Simulates a disconnect failure. Fails if the service doesn't disconnect.

    """
    def _GetTestModemFunctorName(self):
        return 'GetModemDisconnectFailOther'


    def _RunTest(self):
        # Connect to the service.
        service = self.test_env.shill.find_cellular_service_object()
        self.test_env.shill.connect_service_synchronous(
                service, CONNECT_DISCONNECT_TIMEOUT)

        # Disconnect attempt should fail.
        self.test_env.shill.disconnect_service_synchronous(
                service, CONNECT_DISCONNECT_TIMEOUT)

        # Service should be cleaned up as if disconnect succeeded.
        if not self.IsServiceDisconnected():
            raise error.TestError('Service should be disconnected.')


class network_3GDisconnectFailure(test.test):
    """
    The test uses the pseudo modem manager to simulate two failure scenarios of
    a Disconnect call: failure while the modem state is DISCONNECTING and
    failure while it is CONNECTED. The expected behavior of shill is to do
    nothing if the modem state is DISCONNECTING and to clean up the service
    otherwise.

    """
    version = 1

    def run_once(self, pseudomodem_family='3GPP'):
        tests = [
                DisconnectWhileStateIsDisconnectingTest(self,
                                                        pseudomodem_family),
                DisconnectWhileDisconnectInProgressTest(self,
                                                        pseudomodem_family),
                DisconnectFailOtherTest(self, pseudomodem_family),
        ]

        for test in tests:
            test.Run()
