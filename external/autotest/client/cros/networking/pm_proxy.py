# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
This module provides bindings for PseudoModem Manager.

"""

import dbus
import logging

import mm1_proxy

from autotest_lib.client.bin import utils
from autotest_lib.client.cros.cellular import mm1_constants
from autotest_lib.client.cros.cellular.pseudomodem import pm_constants


class PseudoMMProxy(mm1_proxy.ModemManager1Proxy):
    """A wrapper around a DBus proxy for PseudoModem Manager."""

    # Used for software message propagation latencies.
    SHORT_TIMEOUT_SECONDS = 2

    @property
    def iface_testing(self):
        """@return org.chromium.Pseudomodem.Testing DBus interface."""
        return dbus.Interface(
                self._bus.get_object(mm1_constants.I_MODEM_MANAGER,
                                     pm_constants.TESTING_PATH),
                pm_constants.I_TESTING)


    def iface_ism(self, machine_name, timeout_seconds=SHORT_TIMEOUT_SECONDS):
        """
        Get the testing interface of the given interactive state machine.

        @param machine_name: The name of the interactive state machine.
        @param timeout_seconds: Max number of seconds to wait until interactive
            state machine becomes available.
        @return dbus.Interface for the testing interface of
            InteractiveScanningMachine.
        @raise mm1_proxy.ModemManager1ProxyError if a valid DBus object can't
            be found.

        """
        def _get_machine(ignore_error):
            machine = self._bus.get_object(
                    mm1_constants.I_MODEM_MANAGER,
                    '/'.join([pm_constants.TESTING_PATH, machine_name]))
            if machine is None:
                return None

            i_machine = dbus.Interface(machine, pm_constants.I_TESTING_ISM)
            # Only way to know if this DBus object is valid is to call a
            # method on it.
            try:
                i_machine.IsWaiting()  # Ignore result.
                return i_machine
            except dbus.exceptions.DBusException as e:
                if ignore_error:
                    return None
                logging.debug(e)
                raise mm1_proxy.ModemManager1ProxyError(
                        'Failed to obtain a valid object for interactive '
                        'state machine %s. DBus error: %s',
                        machine_name,
                        repr(e))

        try:
            utils.poll_for_condition(
                lambda: _get_machine(True), timeout=timeout_seconds)
        except utils.TimeoutError as e:
            pass

        return _get_machine(False)
