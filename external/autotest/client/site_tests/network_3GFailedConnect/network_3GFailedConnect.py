# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import dbus
import logging

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.cellular.pseudomodem import modem_3gpp
from autotest_lib.client.cros.cellular.pseudomodem import modem_cdma
from autotest_lib.client.cros.cellular.pseudomodem import pm_errors
from autotest_lib.client.cros.cellular.pseudomodem import utils as pm_utils
from autotest_lib.client.cros.networking import cellular_proxy
from autotest_lib.client.cros.networking import shill_proxy


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


def GetFailConnectModem(family):
    """
    Returns the correct modem subclass based on |family|.

    @param family: A string containing either '3GPP' or 'CDMA'.

    """
    modem_class = _GetModemSuperClass(family)

    class FailConnectModem(modem_class):
        """Custom fake Modem that always fails to connect."""
        @pm_utils.log_dbus_method(return_cb_arg='return_cb',
                                  raise_cb_arg='raise_cb')
        def Connect(self, properties, return_cb, raise_cb):
            logging.info('Connect call will fail.')
            raise_cb(pm_errors.MMCoreError(pm_errors.MMCoreError.FAILED))

    return FailConnectModem()


class network_3GFailedConnect(test.test):
    """
    Tests that 3G connect failures are handled by shill properly.

    This test will fail if a connect failure does not immediately cause the
    service to enter the Failed state.

    """
    version = 1

    def _connect_to_3g_network(self, config_timeout):
        """
        Attempts to connect to a 3G network using shill.

        @param config_timeout: Timeout (in seconds) before giving up on
                               connect.

        @raises: error.TestFail if connection fails.

        """
        service = self.test_env.shill.find_cellular_service_object()

        try:
            service.Connect()
        except dbus.DBusException as e:
            logging.info('Expected error: %s', e)

        _, state, _ = self.test_env.shill.wait_for_property_in(
                service,
                shill_proxy.ShillProxy.SERVICE_PROPERTY_STATE,
                ('ready', 'portal', 'online', 'failure'),
                config_timeout)

        if state != 'failure':
            raise error.TestFail('Service state should be failure not %s' %
                                 state)


    def run_once(self, test_env, connect_count=4):
        with test_env:
            self.test_env = test_env
            for count in xrange(connect_count):
                logging.info('Connect attempt %d', count + 1)
                self._connect_to_3g_network(config_timeout=
                        cellular_proxy.CellularProxy.SERVICE_CONNECT_TIMEOUT)
