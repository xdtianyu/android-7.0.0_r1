# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be # found in the LICENSE file.

import dbus
import dbus.service
import logging


class ObjectManager(dbus.service.Object):
    """Exports the org.freedesktop.DBus.ObjectManager interface:

    GetManagedObjects(
            out DICT<OBJPATH,DICT<STRING,DICT<STRING,VARIANT>>>
                    objpath_interfaces_and_properties);

    The return value of this method is a dict whose keys are object
    paths. All returned object paths are children of the object path
    implementing this interface, i.e. their object paths start with
    the ObjectManager's object path plus '/'.

    Each value is a dict whose keys are interfaces names. Each value
    in this inner dict is the same dict that would be returned by the
    org.freedesktop.DBus.Properties.GetAll() method for that
    combination of object path and interface. If an interface has no
    properties, the empty dict is returned.

    Changes are emitted using the following two signals:

    InterfacesAdded(
            OBJPATH object_path,
            DICT<STRING,DICT<STRING,VARIANT>> interfaces_and_properties);

    InterfacesRemoved (OBJPATH object_path, ARRAY<STRING> interfaces);

    """

    OBJECT_MANAGER_INTERFACE = 'org.freedesktop.DBus.ObjectManager'


    def __init__(self, bus, path):
        super(ObjectManager, self).__init__(bus, path)
        self._paths = dict()


    def claim_interface(self, path, interface_name, property_getter):
        """Claim an interface for an object exposed under this ObjectManager.

        @param path: string object path of object exposed.
        @param interface_name: string DBus interface name of exposed object.
        @param property_getter: function that takes no objects and returns
                a dictionary mapping from property name to property value.
                Both property names and values must be DBus types.

        """
        logging.debug('claim_interface(%s, %s, ...)', path, interface_name)
        if path in self._paths:
            self._paths[path][interface_name] = property_getter
        else:
            self._paths[path] = {interface_name: property_getter}
        interface2properties = dbus.Dictionary(
                {dbus.String(interface_name): property_getter()}, 'sa{sv}')
        self.InterfacesAdded(dbus.ObjectPath(path), interface2properties)


    def release_interface(self, path, interface_name):
        """Release an interface previously claimed for a particular |path|.

        You may call this method even if the interface has not been claimed.
        This is intended to simplify destruction patterns.

        @param path: string path exposed previously.
        @param interface_name: string DBus interface name previously claimed.

        """
        logging.debug('release_interface(%s, %s)', path, interface_name)
        if path not in self._paths or interface_name not in self._paths[path]:
            return
        self._paths[path].pop(interface_name)
        if not self._paths[path]:
            self._paths.pop(path)
        self.InterfacesRemoved(dbus.ObjectPath(path),
                               dbus.Array([dbus.String(interface_name)]))


    @dbus.service.method(dbus_interface=OBJECT_MANAGER_INTERFACE,
                         in_signature='', out_signature='a{oa{sa{sv}}}')
    def GetManagedObjects(self):
        """Implementation of DBus interface method of the same name.

        @return see class documentation.

        """
        logging.debug('Received call to GetManagedObjects()')
        result = dbus.Dictionary(dict(), 'oa{sa{sv}}')
        for path, interfaces in self._paths.iteritems():
            dbus_path = dbus.ObjectPath(path)
            result[dbus_path] = dbus.Dictionary(dict(), 'sa{sv}')
            for interface_name, property_getter in interfaces.iteritems():
                dbus_if_name = dbus.String(interface_name)
                result[dbus_path][dbus_if_name] = property_getter()
        return result


    @dbus.service.signal(dbus_interface=OBJECT_MANAGER_INTERFACE,
                         signature='oa{sa{sv}}')
    def InterfacesAdded(self, object_path, interfaces2properties):
        """Implementation of DBus interface signal.

        @param object_path: dbus.ObjectPath object.
        @param interfaces2properties: see class documentation.

        """


    @dbus.service.signal(dbus_interface=OBJECT_MANAGER_INTERFACE,
                         signature='oas')
    def InterfacesRemoved(self, object_path, interfaces):
        """Implementation of DBus interface signal.

        @param object_path: dbus.ObjectPath object.
        @param interfaces: see class documentation.

        """
