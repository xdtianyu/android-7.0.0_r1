# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import dbus
import dbus.service


class NoSuchPropertyException(Exception):
    """Raised when someone requests a property that has not been registered."""
    pass


class DBusPropertyExposer(dbus.service.Object):
    """Exports the org.freedesktop.DBus.Properties interface."""

    def __init__(self, bus, path, interface_name):
        super(DBusPropertyExposer, self).__init__(bus, path)
        self.interface_name = interface_name
        self._properties = dict()


    def register_property(self, property_name, property_getter):
        """Subclasses should call this function for each exposed property.

        @param property_name: string name of property to expose.
        @param property_getter: function that takes no arguments and returns
                a value for the property, in its proper DBus typing.

        """
        self._properties[property_name] = property_getter


    def on_property_changed(self, property_name):
        """Subclasses should call this function when property values change.

        @param property_name: string name of property that changed.

        """
        self.PropertiesChanged(dbus.String(self.interface_name),
                               self.property_getter(),
                               dbus.Array([], 's'))


    def property_getter(self):
        """Method suitable for giving to the ObjectManager.

        @return map of DBus strings to variants.

        """
        results = dbus.Dictionary(dict(), 'sv')
        for property_name, property_getter in self._properties.iteritems():
            results[dbus.String(property_name)] = property_getter()
        return results


    @dbus.service.method(dbus_interface=dbus.PROPERTIES_IFACE,
                         in_signature='ss', out_signature='v')
    def Get(self, interface_name, requested_property_name):
        """Implements org.freedesktop.DBus.Properties.Get().

        @param interface_name: string interface to get properties of.
        @param requested_property_name: string, self explanatory.
        @return variant value of the given property.

        """
        if interface_name != self.interface_name:
            return ''
        for property_name, property_getter in self._properties.iteritems():
            if property_name == requested_property_name:
                return property_getter()
        raise NoSuchPropertyException('Could not find property %s' %
                                      requested_property_name)


    @dbus.service.method(dbus_interface=dbus.PROPERTIES_IFACE,
                         in_signature='s', out_signature='a{sv}')
    def GetAll(self, interface_name):
        """Implements org.freedesktop.DBus.Properties.GetAll().

        @param interface_name: string interface to get properties of.
        @return dict which maps property names to values.

        """
        results = dbus.Dictionary(dict(), 'sv')
        if interface_name != self.interface_name:
            return results
        for property_name, property_getter in self._properties.iteritems():
            results[dbus.String(property_name)] = property_getter()
        return results


    @dbus.service.signal(dbus_interface=dbus.PROPERTIES_IFACE,
                         signature='sa{sv}as')
    def PropertiesChanged(self, interface_name, properties, invalid_properties):
        """Implementation of DBus interface signal.

        org.freedesktop.DBus.Properties.PropertiesChanged (
            STRING interface_name,
            DICT<STRING,VARIANT> changed_properties,
            ARRAY<STRING> invalidated_properties);

        @param interface_name: See above.
        @param properties: See above.
        @param invalid_properties: See above.

        """
