# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import dbus
import logging
import time

from autotest_lib.client.cros.networking import shill_proxy


class CellularProxy(shill_proxy.ShillProxy):
    """Wrapper around shill dbus interface used by cellular tests."""

    # Properties exposed by shill.
    DEVICE_PROPERTY_DBUS_OBJECT = 'DBus.Object'
    DEVICE_PROPERTY_MODEL_ID = 'Cellular.ModelID'
    DEVICE_PROPERTY_OUT_OF_CREDITS = 'Cellular.OutOfCredits'
    DEVICE_PROPERTY_SIM_LOCK_STATUS = 'Cellular.SIMLockStatus'
    DEVICE_PROPERTY_SIM_PRESENT = 'Cellular.SIMPresent'
    DEVICE_PROPERTY_TECHNOLOGY_FAMILY = 'Cellular.Family'
    DEVICE_PROPERTY_TECHNOLOGY_FAMILY_CDMA = 'CDMA'
    DEVICE_PROPERTY_TECHNOLOGY_FAMILY_GSM = 'GSM'
    SERVICE_PROPERTY_LAST_GOOD_APN = 'Cellular.LastGoodAPN'

    # APN info property names.
    APN_INFO_PROPERTY_APN = 'apn'

    # Keys into the dictionaries exposed as properties.
    PROPERTY_KEY_SIM_LOCK_TYPE = 'LockType'
    PROPERTY_KEY_SIM_LOCK_ENABLED = 'LockEnabled'
    PROPERTY_KEY_SIM_LOCK_RETRIES_LEFT = 'RetriesLeft'

    # Valid values taken by properties exposed by shill.
    VALUE_SIM_LOCK_TYPE_PIN = 'sim-pin'
    VALUE_SIM_LOCK_TYPE_PUK = 'sim-puk'

    # Various timeouts in seconds.
    SERVICE_CONNECT_TIMEOUT = 60
    SERVICE_DISCONNECT_TIMEOUT = 60
    SERVICE_REGISTRATION_TIMEOUT = 60
    SLEEP_INTERVAL = 0.1

    def set_logging_for_cellular_test(self):
        """Set the logging in shill for a test of cellular technology.

        Set the log level to |ShillProxy.LOG_LEVEL_FOR_TEST| and the log scopes
        to the ones defined in |ShillProxy.LOG_SCOPES_FOR_TEST| for
        |ShillProxy.TECHNOLOGY_CELLULAR|.

        """
        self.set_logging_for_test(self.TECHNOLOGY_CELLULAR)


    def find_cellular_service_object(self):
        """Returns the first dbus object found that is a cellular service.

        @return DBus object for the first cellular service found. None if no
                service found.

        """
        return self.find_object('Service', {'Type': self.TECHNOLOGY_CELLULAR})


    def wait_for_cellular_service_object(
            self, timeout_seconds=SERVICE_REGISTRATION_TIMEOUT):
        """Waits for the cellular service object to show up.

        @param timeout_seconds: Amount of time to wait for cellular service.
        @return DBus object for the first cellular service found.
        @raises ShillProxyError if no cellular service is found within the
            registration timeout period.

        """
        CellularProxy._poll_for_condition(
                lambda: self.find_cellular_service_object() is not None,
                'Failed to find cellular service object',
                timeout=timeout_seconds)
        return self.find_cellular_service_object()


    def find_cellular_device_object(self):
        """Returns the first dbus object found that is a cellular device.

        @return DBus object for the first cellular device found. None if no
                device found.

        """
        return self.find_object('Device', {'Type': self.TECHNOLOGY_CELLULAR})


    def reset_modem(self, modem, expect_device=True, expect_powered=True,
                    expect_service=True):
        """Reset |modem|.

        Do, in sequence,
        (1) Ensure that the current device object disappears.
        (2) If |expect_device|, ensure that the device reappears.
        (3) If |expect_powered|, ensure that the device is powered.
        (4) If |expect_service|, ensure that the service reappears.

        This function does not check the service state for the device after
        reset.

        @param modem: DBus object for the modem to reset.
        @param expect_device: If True, ensure that a DBus object reappears for
                the same modem after the reset.
        @param expect_powered: If True, ensure that the modem is powered on
                after the reset.
        @param expect_service: If True, ensure that a service managing the
                reappeared modem also reappears.

        @return (device, service)
                device: DBus object for the reappeared Device after the reset.
                service: DBus object for the reappeared Service after the reset.
                Either of these may be None, if the object is not expected to
                reappear.

        @raises ShillProxyError if any of the conditions (1)-(4) fail.

        """
        logging.info('Resetting modem')
        # Obtain identifying information about the modem.
        properties = modem.GetProperties(utf8_strings=True)
        # NOTE: Using the Model ID means that this will break if we have two
        # identical cellular modems in a DUT. Fortunately, we only support one
        # modem at a time.
        model_id = properties.get(self.DEVICE_PROPERTY_MODEL_ID)
        if not model_id:
            raise shill_proxy.ShillProxyError(
                    'Failed to get identifying information for the modem.')
        old_modem_path = modem.object_path
        old_modem_mm_object = properties.get(self.DEVICE_PROPERTY_DBUS_OBJECT)
        if not old_modem_mm_object:
            raise shill_proxy.ShillProxyError(
                    'Failed to get the mm object path for the modem.')

        modem.Reset()

        # (1) Wait for the old modem to disappear
        CellularProxy._poll_for_condition(
                lambda: self._is_old_modem_gone(old_modem_path,
                                                old_modem_mm_object),
                'Old modem disappeared',
                timeout=60)

        # (2) Wait for the device to reappear
        if not expect_device:
            return None, None
        # The timeout here should be sufficient for our slowest modem to
        # reappear.
        CellularProxy._poll_for_condition(
                lambda: self._get_reappeared_modem(model_id,
                                                   old_modem_mm_object),
                desc='The modem reappeared after reset.',
                timeout=60)
        new_modem = self._get_reappeared_modem(model_id, old_modem_mm_object)

        # (3) Check powered state of the device
        if not expect_powered:
            return new_modem, None
        success, _, _ = self.wait_for_property_in(new_modem,
                                                  self.DEVICE_PROPERTY_POWERED,
                                                  [self.VALUE_POWERED_ON],
                                                  timeout_seconds=10)
        if not success:
            raise shill_proxy.ShillProxyError(
                    'After modem reset, new modem failed to enter powered '
                    'state.')

        # (4) Check that service reappears
        if not expect_service:
            return new_modem, None
        new_service = self.get_service_for_device(new_modem)
        if not new_service:
            raise shill_proxy.ShillProxyError(
                    'Failed to find a shill service managing the reappeared '
                    'device.')
        return new_modem, new_service


    def disable_modem_for_test_setup(self, timeout_seconds=10):
        """
        Disables all cellular modems.

        Use this method only for setting up tests.  Do not use this method to
        test disable functionality because this method repeatedly attempts to
        disable the cellular technology until it succeeds (ignoring all DBus
        errors) since the DisableTechnology() call may fail for various reasons
        (eg. an enable is in progress).

        @param timeout_seconds: Amount of time to wait until the modem is
                disabled.
        @raises ShillProxyError if the modems fail to disable within
                |timeout_seconds|.

        """
        def _disable_cellular_technology(self):
            try:
                self._manager.DisableTechnology(self.TECHNOLOGY_CELLULAR)
                return True
            except dbus.DBusException as e:
                return False

        CellularProxy._poll_for_condition(
                lambda: _disable_cellular_technology(self),
                'Failed to disable cellular technology.',
                timeout=timeout_seconds)
        modem = self.find_cellular_device_object()
        self.wait_for_property_in(modem, self.DEVICE_PROPERTY_POWERED,
                                  [self.VALUE_POWERED_OFF],
                                  timeout_seconds=timeout_seconds)


    # TODO(pprabhu) Use utils.poll_for_condition instead
    @staticmethod
    def _poll_for_condition(condition, desc, timeout=10):
        """Poll till |condition| is satisfied.

        @param condition: A function taking no arguments. The condition is
                met when the return value can be cast to the bool True.
        @param desc: The description given when we timeout waiting for
                |condition|.

        """
        start_time = time.time()
        while True:
            value = condition()
            if value:
                return value
            if(time.time() + CellularProxy.SLEEP_INTERVAL - start_time >
               timeout):
                raise shill_proxy.ShillProxyError(
                        'Timed out waiting for condition %s.' % desc)
            time.sleep(CellularProxy.SLEEP_INTERVAL)


    def _is_old_modem_gone(self, modem_path, modem_mm_object):
        """Tests if the DBus object for modem disappears after Reset.

        @param modem_path: The DBus path for the modem object that must vanish.
        @param modem_mm_object: The modemmanager object path reported by the
            old modem. This is unique everytime a new modem is (re)exposed.

        @return True if the object disappeared, false otherwise.

        """
        device = self.get_dbus_object(self.DBUS_TYPE_DEVICE, modem_path)
        try:
            properties = device.GetProperties()
            # DBus object exists, perhaps a reappeared device?
            return (properties.get(self.DEVICE_PROPERTY_DBUS_OBJECT) !=
                    modem_mm_object)
        except dbus.DBusException as e:
            if e.get_dbus_name() == self.DBUS_ERROR_UNKNOWN_OBJECT:
                return True
            return False


    def _get_reappeared_modem(self, model_id, old_modem_mm_object):
        """Check that a vanished modem reappers.

        @param model_id: The model ID reported by the vanished modem.
        @param old_modem_mm_object: The previously reported modemmanager object
                path for this modem.

        @return The reappeared DBus object, if any. None otherwise.

        """
        # TODO(pprabhu) This will break if we have multiple cellular devices
        # in the system at the same time.
        device = self.find_cellular_device_object()
        if not device:
            return None
        properties = device.GetProperties(utf8_strings=True)
        if (model_id == properties.get(self.DEVICE_PROPERTY_MODEL_ID) and
            (old_modem_mm_object !=
             properties.get(self.DEVICE_PROPERTY_DBUS_OBJECT))):
            return device
        return None
