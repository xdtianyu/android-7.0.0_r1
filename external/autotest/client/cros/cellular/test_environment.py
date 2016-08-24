# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import contextlib
import dbus
import logging
import sys
import traceback

import common
from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import backchannel
from autotest_lib.client.cros.cellular import cell_tools
from autotest_lib.client.cros.cellular import mm
from autotest_lib.client.cros.cellular.pseudomodem import pseudomodem_context
from autotest_lib.client.cros.cellular.wardmodem import wardmodem
from autotest_lib.client.cros.networking import cellular_proxy
from autotest_lib.client.cros.networking import shill_proxy

# Import 'flimflam_test_path' first in order to import flimflam.
# pylint: disable=W0611
from autotest_lib.client.cros import flimflam_test_path
import flimflam

class CellularTestEnvironment(object):
    """Setup and verify cellular test environment.

    This context manager configures the following:
        - Sets up backchannel.
        - Shuts down other devices except cellular.
        - Shill and MM logging is enabled appropriately for cellular.
        - Initializes members that tests should use to access test environment
          (eg. |shill|, |flimflam|, |modem_manager|, |modem|).

    Then it verifies the following is valid:
        - The backchannel is using an Ethernet device.
        - The SIM is inserted and valid.
        - There is one and only one modem in the device.
        - The modem is registered to the network.
        - There is a cellular service in shill and it's not connected.

    Don't use this base class directly, use the appropriate subclass.

    Setup for over-the-air tests:
        with CellularOTATestEnvironment() as test_env:
            # Test body

    Setup for pseudomodem tests:
        with CellularPseudoMMTestEnvironment(
                pseudomm_args=({'family': '3GPP'})) as test_env:
            # Test body

    Setup for wardmodem tests:
        with CellularWardModemTestEnvironment(
                wardmodem_modem='e362') as test_env:
            # Test body

    """

    def __init__(self, use_backchannel=True, shutdown_other_devices=True,
                 modem_pattern=''):
        """
        @param use_backchannel: Set up the backchannel that can be used to
                communicate with the DUT.
        @param shutdown_other_devices: If True, shutdown all devices except
                cellular.
        @param modem_pattern: Search string used when looking for the modem.

        """
        # Tests should use this main loop instead of creating their own.
        self.mainloop = dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)
        self.bus = dbus.SystemBus(mainloop=self.mainloop)

        self.shill = None
        self.flim = None  # Only use this for legacy tests.
        self.modem_manager = None
        self.modem = None
        self.modem_path = None
        self._backchannel = None

        self._modem_pattern = modem_pattern

        self._nested = None
        self._context_managers = []
        if use_backchannel:
            self._backchannel = backchannel.Backchannel()
            self._context_managers.append(self._backchannel)
        if shutdown_other_devices:
            self._context_managers.append(
                    cell_tools.OtherDeviceShutdownContext('cellular'))


    @contextlib.contextmanager
    def _disable_shill_autoconnect(self):
        self._enable_shill_cellular_autoconnect(False)
        yield
        self._enable_shill_cellular_autoconnect(True)


    def __enter__(self):
        try:
            # Temporarily disable shill autoconnect to cellular service while
            # the test environment is setup to prevent a race condition
            # between disconnecting the modem in _verify_cellular_service()
            # and shill autoconnect.
            with self._disable_shill_autoconnect():
                self._nested = contextlib.nested(*self._context_managers)
                self._nested.__enter__()

                self._initialize_shill()

                # Perform SIM verification now to ensure that we can enable the
                # modem in _initialize_modem_components(). ModemManager does not
                # allow enabling a modem without a SIM.
                self._verify_sim()
                self._initialize_modem_components()

                self._setup_logging()

                self._verify_backchannel()
                self._wait_for_modem_registration()
                self._verify_cellular_service()

                return self
        except (error.TestError, dbus.DBusException,
                shill_proxy.ShillProxyError) as e:
            except_type, except_value, except_traceback = sys.exc_info()
            lines = traceback.format_exception(except_type, except_value,
                                               except_traceback)
            logging.error('Error during test initialization:\n' +
                          ''.join(lines))
            self.__exit__(*sys.exc_info())
            raise error.TestError('INIT_ERROR: %s' % str(e))
        except:
            self.__exit__(*sys.exc_info())
            raise


    def __exit__(self, exception, value, traceback):
        if self._nested:
            return self._nested.__exit__(exception, value, traceback)
        self.shill = None
        self.flim = None
        self.modem_manager = None
        self.modem = None
        self.modem_path = None


    def _get_shill_cellular_device_object(self):
        modem_device = self.shill.find_cellular_device_object()
        if not modem_device:
            raise error.TestError('Cannot find cellular device in shill. '
                                  'Is the modem plugged in?')
        return modem_device


    def _enable_modem(self):
        modem_device = self._get_shill_cellular_device_object()
        try:
            modem_device.Enable()
        except dbus.DBusException as e:
            if (e.get_dbus_name() !=
                    shill_proxy.ShillProxy.ERROR_IN_PROGRESS):
                raise

        utils.poll_for_condition(
            lambda: modem_device.GetProperties()['Powered'],
            exception=error.TestError(
                    'Failed to enable modem.'),
            timeout=shill_proxy.ShillProxy.DEVICE_ENABLE_DISABLE_TIMEOUT)


    def _enable_shill_cellular_autoconnect(self, enable):
        shill = cellular_proxy.CellularProxy.get_proxy(self.bus)
        shill.manager.SetProperty(
                shill_proxy.ShillProxy.
                MANAGER_PROPERTY_NO_AUTOCONNECT_TECHNOLOGIES,
                '' if enable else 'cellular')


    def _is_unsupported_error(self, e):
        return (e.get_dbus_name() ==
                shill_proxy.ShillProxy.ERROR_NOT_SUPPORTED or
                (e.get_dbus_name() ==
                 shill_proxy.ShillProxy.ERROR_FAILURE and
                 'operation not supported' in e.get_dbus_message()))


    def _reset_modem(self):
        modem_device = self._get_shill_cellular_device_object()
        try:
            # Cromo/MBIM modems do not support being reset.
            self.shill.reset_modem(modem_device, expect_service=False)
        except dbus.DBusException as e:
            if not self._is_unsupported_error(e):
                raise


    def _initialize_shill(self):
        """Get access to shill."""
        # CellularProxy.get_proxy() checks to see if shill is running and
        # responding to DBus requests. It returns None if that's not the case.
        self.shill = cellular_proxy.CellularProxy.get_proxy(self.bus)
        if self.shill is None:
            raise error.TestError('Cannot connect to shill, is shill running?')

        # Keep this around to support older tests that haven't migrated to
        # cellular_proxy.
        self.flim = flimflam.FlimFlam()


    def _initialize_modem_components(self):
        """Reset the modem and get access to modem components."""
        # Enable modem first so shill initializes the modemmanager proxies so
        # we can call reset on it.
        self._enable_modem()
        self._reset_modem()

        # PickOneModem() makes sure there's a modem manager and that there is
        # one and only one modem.
        self.modem_manager, self.modem_path = \
                mm.PickOneModem(self._modem_pattern)
        self.modem = self.modem_manager.GetModem(self.modem_path)
        if self.modem is None:
            raise error.TestError('Cannot get modem object at %s.' %
                                  self.modem_path)


    def _setup_logging(self):
        self.shill.set_logging_for_cellular_test()
        self.modem_manager.SetDebugLogging()


    def _verify_sim(self):
        """Verify SIM is valid.

        Make sure a SIM in inserted and that it is not locked.

        @raise error.TestError if SIM does not exist or is locked.

        """
        modem_device = self._get_shill_cellular_device_object()
        props = modem_device.GetProperties()

        # No SIM in CDMA modems.
        family = props[
                cellular_proxy.CellularProxy.DEVICE_PROPERTY_TECHNOLOGY_FAMILY]
        if (family ==
                cellular_proxy.CellularProxy.
                DEVICE_PROPERTY_TECHNOLOGY_FAMILY_CDMA):
            return

        # Make sure there is a SIM.
        if not props[cellular_proxy.CellularProxy.DEVICE_PROPERTY_SIM_PRESENT]:
            raise error.TestError('There is no SIM in the modem.')

        # Make sure SIM is not locked.
        lock_status = props.get(
                cellular_proxy.CellularProxy.DEVICE_PROPERTY_SIM_LOCK_STATUS,
                None)
        if lock_status is None:
            raise error.TestError('Failed to read SIM lock status.')
        locked = lock_status.get(
                cellular_proxy.CellularProxy.PROPERTY_KEY_SIM_LOCK_ENABLED,
                None)
        if locked is None:
            raise error.TestError('Failed to read SIM LockEnabled status.')
        elif locked:
            raise error.TestError(
                    'SIM is locked, test requires an unlocked SIM.')


    def _verify_backchannel(self):
        """Verify backchannel is on an ethernet device.

        @raise error.TestError if backchannel is not on an ethernet device.

        """
        if self._backchannel is None:
            return

        if not self._backchannel.is_using_ethernet():
            raise error.TestError('An ethernet connection is required between '
                                  'the test server and the device under test.')


    def _wait_for_modem_registration(self):
        """Wait for the modem to register with the network.

        @raise error.TestError if modem is not registered.

        """
        utils.poll_for_condition(
            self.modem.ModemIsRegistered,
            exception=error.TestError(
                    'Modem failed to register with the network.'),
            timeout=cellular_proxy.CellularProxy.SERVICE_REGISTRATION_TIMEOUT)


    def _verify_cellular_service(self):
        """Make sure a cellular service exists.

        The cellular service should not be connected to the network.

        @raise error.TestError if cellular service does not exist or if
                there are multiple cellular services.

        """
        service = self.shill.wait_for_cellular_service_object()

        try:
            service.Disconnect()
        except dbus.DBusException as e:
            if (e.get_dbus_name() !=
                    cellular_proxy.CellularProxy.ERROR_NOT_CONNECTED):
                raise
        success, state, _ = self.shill.wait_for_property_in(
                service,
                cellular_proxy.CellularProxy.SERVICE_PROPERTY_STATE,
                ('idle',),
                cellular_proxy.CellularProxy.SERVICE_DISCONNECT_TIMEOUT)
        if not success:
            raise error.TestError(
                    'Cellular service needs to start in the "idle" state. '
                    'Current state is "%s". '
                    'Modem disconnect may have failed.' %
                    state)


class CellularOTATestEnvironment(CellularTestEnvironment):
    """Setup and verify cellular over-the-air (OTA) test environment. """
    def __init__(self, **kwargs):
        super(CellularOTATestEnvironment, self).__init__(**kwargs)


class CellularPseudoMMTestEnvironment(CellularTestEnvironment):
    """Setup and verify cellular pseudomodem test environment. """
    def __init__(self, pseudomm_args=None, **kwargs):
        """
        @param pseudomm_args: Tuple of arguments passed to the pseudomodem, see
                pseudomodem_context.py for description of each argument in the
                tuple: (flags_map, block_output, bus)

        """
        super(CellularPseudoMMTestEnvironment, self).__init__(**kwargs)
        self._context_managers.append(
                pseudomodem_context.PseudoModemManagerContext(
                        True, bus=self.bus, *pseudomm_args))


class CellularWardModemTestEnvironment(CellularTestEnvironment):
    """Setup and verify cellular ward modem test environment. """
    def __init__(self, wardmodem_modem=None, **kwargs):
        """
        @param wardmodem_modem: Customized ward modem to use instead of the
                default implementation, see wardmodem.py.

        """
        super(CellularWardModemTestEnvironment, self).__init__(**kwargs)
        self._context_managers.append(
                wardmodem.WardModemContext(args=['--modem', wardmodem_modem]))
