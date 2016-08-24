# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import dbus
import dbus.mainloop.glib
import time

from autotest_lib.client.common_lib.cros.network import apmanager_constants
from autotest_lib.client.cros import dbus_util


class ApmanagerProxyError(Exception):
    """Exceptions raised by ApmanagerProxy and it's children."""
    pass


class ApmanagerProxy(object):
    """A wrapper around a DBus proxy for apmanager."""

    # Core DBus error names
    DBUS_ERROR_UNKNOWN_OBJECT = 'org.freedesktop.DBus.Error.UnknownObject'
    DBUS_ERROR_SERVICE_UNKNOWN = 'org.freedesktop.DBus.Error.ServiceUnknown'
    DBUS_ERROR_UNKNOWN_METHOD = 'org.freedesktop.DBus.Error.UnknownMethod'

    # apmanager Service and Interface names.
    DBUS_SERVICE = 'org.chromium.apmanager'
    DBUS_PROPERTY_INTERFACE = 'org.freedesktop.DBus.Properties'
    DBUS_CONFIG_INTERFACE = 'org.chromium.apmanager.Config'
    DBUS_SERVICE_INTERFACE = 'org.chromium.apmanager.Service'
    DBUS_MANAGER_INTERFACE = 'org.chromium.apmanager.Manager'
    DBUS_MANAGER_PATH = '/org/chromium/apmanager/Manager'

    # AP Service property keys
    SERVICE_PROPERTY_CONFIG = 'Config'

    # Mapping for property to dbus type function.
    CONFIG_PROPERTY_DBUS_TYPE_MAPPING = {
            apmanager_constants.CONFIG_BRIDGE_INTERFACE: dbus.String,
            apmanager_constants.CONFIG_CHANNEL: dbus.UInt16,
            apmanager_constants.CONFIG_HIDDEN_NETWORK: dbus.Boolean,
            apmanager_constants.CONFIG_HW_MODE: dbus.String,
            apmanager_constants.CONFIG_INTERFACE_NAME: dbus.String,
            apmanager_constants.CONFIG_OPERATION_MODE: dbus.String,
            apmanager_constants.CONFIG_PASSPHRASE: dbus.String,
            apmanager_constants.CONFIG_SECURITY_MODE: dbus.String,
            apmanager_constants.CONFIG_SERVER_ADDRESS_INDEX: dbus.UInt16,
            apmanager_constants.CONFIG_SSID: dbus.String}

    POLLING_INTERVAL_SECONDS = 0.2


    def __init__(self, bus=None, timeout_seconds=10):
        if bus is None:
            dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)
            bus = dbus.SystemBus()
        self._bus = bus
        self._manager = None
        self._connect_to_dbus(timeout_seconds)


    def _connect_to_dbus(self, timeout_seconds):
        """Connect to apmanager over DBus and initialize the DBus object for
           org.chromium.apmanager.Manager interface.

        If apmanager is not yet running, retry until it is, or until
        |timeout_seconds| expires.

        @param timeout_seconds float number of seconds to wait for connecting
               to apmanager's DBus service.

        """
        end_time = time.time() + timeout_seconds
        while self._manager is None and time.time() < end_time:
            try:
                self._manager = \
                        self._get_dbus_object(self.DBUS_MANAGER_INTERFACE,
                                              self.DBUS_MANAGER_PATH)
            except dbus.exceptions.DBusException as e:
                if (e.get_dbus_name() !=
                    ApmanagerProxy.DBUS_ERROR_SERVICE_UNKNOWN):
                    raise ApmanagerProxyError('Error connecting to apmanager')
                else:
                    # Wait a moment before retrying
                    time.sleep(ApmanagerProxy.POLLING_INTERVAL_SECONDS)
        if self._manager is None:
            raise ApmanagerProxyError('Timeout connecting to apmanager')


    def _get_dbus_object(self, interface_name, path):
        """Return the DBus object of interface |interface_name| at |path| in
           apmanager DBUS service.

        @param interface_name string (e.g. self.DBUS_SERVICE_INTERFACE).
        @param path path to object in apmanager (e.g. '/manager/services/1').
        @return DBus proxy object.

        """
        return dbus.Interface(
                self._bus.get_object(self.DBUS_SERVICE, path),
                interface_name)


    def _get_dbus_property(self, dbus_object, interface_name, property_key):
        """get property on a dbus Interface

        @param dbus_object DBus object to read property from
        @param interface_name string name of the interface
        @param property_key string name of property on interface
        @return python typed object representing property value or None

        """
        # Get the property interface for the given DBus object.
        property_interface = self._get_dbus_object(
                self.DBUS_PROPERTY_INTERFACE,
                dbus_object.object_path)
        # Invoke Get method on the property interface.
        try:
            value = dbus_util.dbus2primitive(
                    property_interface.Get(dbus.String(interface_name),
                                           dbus.String(property_key)));
        except dbus.exceptions.DBusException as e:
            raise ApmanagerProxyError(
                    'Failed to get property %s on interface %s' %
                    (property_key, interface_name))
        return value


    def _set_dbus_property(self,
                           dbus_object,
                           interface_name,
                           property_key,
                           value):
        """set property on a dbus Interface

        @param dbus_object DBus object to set property on
        @param interface_name string name of the interface
        @param property_key string name of property on interface
        @param value dbus_type value to set for property

        """
        # Get the property interface for the given DBus object.
        property_interface = self._get_dbus_object(
                self.DBUS_PROPERTY_INTERFACE,
                dbus_object.object_path)
        # Invoke Set method on the property interface.
        try:
            property_interface.Set(dbus.String(interface_name),
                                   dbus.String(property_key),
                                   value);
        except dbus.exceptions.DBusException as e:
            raise ApmanagerProxyError(
                    'Failed to set property %s on interface %s' %
                    (property_key, interface_name))


    # TODO(zqiu): add more optional parameters for setting additional
    # service configurations.
    def start_service(self, config_params):
        """Create/start an AP service with provided configurations.

        @param config_params dictionary of configuration parameters.
        @return string object path of the newly created service.

        """
        service = self._get_dbus_object(
                self.DBUS_SERVICE_INTERFACE,
                dbus_util.dbus2primitive(self._manager.CreateService()))
        # Get configuration object for the service.
        service_config = self._get_dbus_object(
                self.DBUS_CONFIG_INTERFACE,
                self._get_dbus_property(service,
                                        self.DBUS_SERVICE_INTERFACE,
                                        self.SERVICE_PROPERTY_CONFIG))
        # Set configuration properties.
        for name, value in config_params.iteritems():
            if name in self.CONFIG_PROPERTY_DBUS_TYPE_MAPPING:
                func = self.CONFIG_PROPERTY_DBUS_TYPE_MAPPING[name]
                self._set_dbus_property(service_config,
                                        self.DBUS_CONFIG_INTERFACE,
                                        name,
                                        func(value, variant_level=1))
            else:
                raise ApmanagerProxyError('Unknown configuration parameter [%s]'
                                          % name)

        # Start AP service.
        service.Start()
        return service.object_path


    def terminate_service(self, service_path):
        """ Terminate and remove the AP service |service|.

        @param service_path string object path of the service.

        """
        self._manager.RemoveService(dbus.ObjectPath(service_path))
