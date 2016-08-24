# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
Python implementation of the standard interfaces:
  - org.freedesktop.DBus.Properties
  - org.freedesktop.DBus.Introspectable (TODO(armansito): May not be necessary)
  - org.freedesktop.DBus.ObjectManager

"""

import dbus
import dbus.service
import dbus.types
import logging

import pm_errors
import utils

from autotest_lib.client.cros.cellular import mm1_constants

class MMPropertyError(pm_errors.MMError):
    """
    MMPropertyError is raised by DBusProperties methods
    to indicate that a value for the given interface or
    property could not be found.

    """

    UNKNOWN_PROPERTY = 0
    UNKNOWN_INTERFACE = 1

    def __init__(self, errno, *args, **kwargs):
        super(MMPropertyError, self).__init__(errno, args, kwargs)


    def _Setup(self):
        self._error_name_base = mm1_constants.I_MODEM_MANAGER
        self._error_name_map = {
            self.UNKNOWN_PROPERTY : '.UnknownProperty',
            self.UNKNOWN_INTERFACE : '.UnknownInterface'
        }


class DBusProperties(dbus.service.Object):
    """
    == org.freedesktop.DBus.Properties ==

    This serves as the abstract base class for all objects that expose
    properties. Each instance holds a mapping from DBus interface names to
    property-value mappings, which are provided by the subclasses.

    """

    def __init__(self, path, bus=None, config=None):
        """
        @param bus: The pydbus bus object.
        @param path: The DBus object path of this object.
        @param config: This is an optional dictionary that can be used to
                initialize the property dictionary with values other than the
                ones provided by |_InitializeProperties|. The dictionary has to
                contain a mapping from DBus interfaces to property-value pairs,
                and all contained keys must have been initialized during
                |_InitializeProperties|, i.e. if config contains any keys that
                have not been already set in the internal property dictionary,
                an error will be raised (see DBusProperties.Set).

        """
        if not path:
            raise TypeError(('A value for "path" has to be provided that is '
                'not "None".'))
        if bus:
          dbus.service.Object.__init__(self, bus, path)
        else:
          dbus.service.Object.__init__(self, None, None)
        self.path = path
        self.bus = bus
        self._properties = self._InitializeProperties()

        if config:
            for key, props in config:
                for prop, val in props:
                    self.Set(key, prop, val)


    @property
    def properties(self):
        """
        @returns: The property dictionary.

        """
        return self._properties


    def SetBus(self, bus):
        """
        Sets the pydbus bus object that this instance of DBusProperties should
        be exposed on. Call this method only if |bus| is not already set.

        @param bus: The pydbus bus object to assign.

        """
        self.bus = bus
        self.add_to_connection(bus, self.path)


    def SetPath(self, path):
        """
        Exposes this object on a new DBus path. This method fails with an
        Exception by default, since exposing an object on multiple paths is
        disallowed by default.

        Subclasses can change this behavior by setting the
        SUPPORTS_MULTIPLE_OBJECT_PATHS class variable to True.

        @param path: The new path to assign to this object.

        """
        self.path = path
        self.add_to_connection(self.bus, path)


    def SetUInt32(self, interface_name, property_name, value):
        """
        Sets the given uint32 value matching the given property and interface.
        Wraps the given value inside a dbus.types.UInt32.

        @param interface_name: The DBus interface name.
        @param property_name: The property name.
        @param value: Value to set.
        @raises: MMPropertyError, if the given |interface_name| or
                |property_name| is not exposed by this object.
        Emits:
            PropertiesChanged

        """
        self.Set(interface_name, property_name, dbus.types.UInt32(value))


    def SetInt32(self, interface_name, property_name, value):
        """
        Sets the given int32 value matching the given property and interface.
        Wraps the given value inside a dbus.types.Int32.

        @param interface_name: The DBus interface name.
        @param property_name: The property name.
        @param value: Value to set.
        @raises: MMPropertyError, if the given |interface_name| or
                |property_name| is not exposed by this object.
        Emits:
            PropertiesChanged

        """
        self.Set(interface_name, property_name, dbus.types.Int32(value))


    @utils.log_dbus_method()
    @dbus.service.method(mm1_constants.I_PROPERTIES, in_signature='ss',
                         out_signature='v')
    def Get(self, interface_name, property_name):
        """
        Returns the value matching the given property and interface.

        @param interface_name: The DBus interface name.
        @param property_name: The property name.
        @returns: The value matching the given property and interface.
        @raises: MMPropertyError, if the given |interface_name| or
                |property_name| is not exposed by this object.

        """
        logging.info(
            '%s: Get(%s, %s)',
            self.path,
            interface_name,
            property_name)
        val = self.GetAll(interface_name).get(property_name, None)
        if val is None:
            message = ("Property '%s' not implemented for interface '%s'." %
                (property_name, interface_name))
            logging.info(message)
            raise MMPropertyError(
                MMPropertyError.UNKNOWN_PROPERTY, message)
        return val


    @utils.log_dbus_method()
    @dbus.service.method(mm1_constants.I_PROPERTIES, in_signature='ssv')
    def Set(self, interface_name, property_name, value):
        """
        Sets the value matching the given property and interface.

        @param interface_name: The DBus interface name.
        @param property_name: The property name.
        @param value: The value to set.
        @raises: MMPropertyError, if the given |interface_name| or
                |property_name| is not exposed by this object.
        Emits:
            PropertiesChanged

        """
        logging.info(
            '%s: Set(%s, %s)',
            self.path,
            interface_name,
            property_name)
        props = self.GetAll(interface_name)
        if property_name not in props:
            raise MMPropertyError(
                MMPropertyError.UNKNOWN_PROPERTY,
                ("Property '%s' not implemented for "
                "interface '%s'.") %
                (property_name, interface_name))
        if props[property_name] == value:
            logging.info("Property '%s' already has value '%s'. Ignoring.",
                         property_name,
                         value)
            return
        props[property_name] = value
        changed = { property_name : value }
        inv = self._InvalidatedPropertiesForChangedValues(changed)
        self.PropertiesChanged(interface_name, changed, inv)


    @utils.log_dbus_method()
    @dbus.service.method(mm1_constants.I_PROPERTIES,
                         in_signature='s', out_signature='a{sv}')
    def GetAll(self, interface_name):
        """
        Returns all property-value pairs that match the given interface.

        @param interface_name: The DBus interface name.
        @returns: A dictionary, containing the properties of the given DBus
                interface and their values.
        @raises: MMPropertyError, if the given |interface_name| or
                |property_name| is not exposed by this object.

        """
        logging.info(
            '%s: GetAll(%s)',
            self.path,
            interface_name)
        props = self._properties.get(interface_name, None)
        if props is None:
            raise MMPropertyError(
                MMPropertyError.UNKNOWN_INTERFACE,
                "Object does not implement interface '%s'." %
                interface_name)
        return props


    @dbus.service.signal(mm1_constants.I_PROPERTIES, signature='sa{sv}as')
    def PropertiesChanged(
            self,
            interface_name,
            changed_properties,
            invalidated_properties):
        """
        This signal is emitted by Set, when the value of a property is changed.

        @param interface_name: The interface the changed properties belong to.
        @param changed_properties: Dictionary containing the changed properties
                                   and their new values.
        @param invalidated_properties: List of properties that were invalidated
                                       when properties changed.

        """
        logging.info(('Properties Changed on interface: %s Changed Properties:'
            ' %s InvalidatedProperties: %s.', interface_name,
            str(changed_properties), str(invalidated_properties)))


    def SetAll(self, interface, properties):
        """
        Sets the entire property dictionary for the given interface.

        @param interface: String specifying the DBus interface.
        @param properties: Dictionary containing the properties to set.
        Emits:
            PropertiesChanged

        """
        old_props = self._properties.get(interface, None)
        if old_props:
            invalidated = old_props.keys()
        else:
            invalidated = []
        self._properties[interface] = properties
        self.PropertiesChanged(interface, properties, invalidated)


    def GetInterfacesAndProperties(self):
        """
        Returns all DBus properties of this object.

        @returns: The complete property dictionary. The returned dict is a tree,
                where the keys are DBus interfaces and the values are
                dictionaries that map properties to values.
        """
        return self._properties


    def _InvalidatedPropertiesForChangedValues(self, changed):
        """
        Called by Set, returns the list of property names that should become
        invalidated given the properties and their new values contained in
        changed. Subclasses can override this method; the default implementation
        returns an empty list.

        """
        return []


    def _InitializeProperties(self):
        """
        Called at instantiation. Subclasses have to override this method and
        return a dictionary containing mappings from implemented interfaces to
        dictionaries of property-value mappings.

        """
        raise NotImplementedError()


class DBusObjectManager(dbus.service.Object):
    """
    == org.freedesktop.DBus.ObjectManager ==

    This interface, included in rev. 0.17 of the DBus specification, allows a
    generic way to control the addition and removal of Modem objects, as well
    as the addition and removal of interfaces in the given objects.

    """

    def __init__(self, bus, path):
        dbus.service.Object.__init__(self, bus, path)
        self.devices = []
        self.bus = bus
        self.path = path


    def Add(self, device):
        """
        Adds a device to the list of devices that are managed by this modem
        manager.

        @param device: Device to add.
        Emits:
            InterfacesAdded

        """
        self.devices.append(device)
        device.manager = self
        self.InterfacesAdded(device.path, device.GetInterfacesAndProperties())


    def Remove(self, device):
        """
        Removes a device from the list of devices that are managed by this
        modem manager.

        @param device: Device to remove.
        Emits:
            InterfacesRemoved

        """
        if device in self.devices:
            self.devices.remove(device)
        interfaces = device.GetInterfacesAndProperties().keys()
        self.InterfacesRemoved(device.path, interfaces)
        device.remove_from_connection()


    @utils.log_dbus_method()
    @dbus.service.method(mm1_constants.I_OBJECT_MANAGER,
                         out_signature='a{oa{sa{sv}}}')
    def GetManagedObjects(self):
        """
        @returns: A dictionary containing all objects and their properties. The
                keys to the dictionary are object paths which are mapped to
                dictionaries containing mappings from DBus interface names to
                property-value pairs.

        """
        results = {}
        for device in self.devices:
            results[dbus.types.ObjectPath(device.path)] = (
                    device.GetInterfacesAndProperties())
        logging.info('%s: GetManagedObjects: %s', self.path,
                     ', '.join(results.keys()))
        return results


    @dbus.service.signal(mm1_constants.I_OBJECT_MANAGER,
                         signature='oa{sa{sv}}')
    def InterfacesAdded(self, object_path, interfaces_and_properties):
        """
        The InterfacesAdded signal is emitted when either a new object is added
        or when an existing object gains one or more interfaces.

        @param object_path: Path of the added object.
        @param interfaces_and_properties: The complete property dictionary that
                belongs to the recently added object.

        """
        logging.info((self.path + ': InterfacesAdded(' + object_path +
                     ', ' + str(interfaces_and_properties)) + ')')


    @dbus.service.signal(mm1_constants.I_OBJECT_MANAGER, signature='oas')
    def InterfacesRemoved(self, object_path, interfaces):
        """
        The InterfacesRemoved signal is emitted whenever an object is removed
        or it loses one or more interfaces.

        @param object_path: Path of the remove object.
        @param interfaces_and_properties: The complete property dictionary that
                belongs to the recently removed object.

        """
        logging.info((self.path + ': InterfacesRemoved(' + object_path +
                     ', ' + str(interfaces) + ')'))
