# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import dbus
import logging


DBUS_INTERFACE_OBJECT_MANAGER = 'org.freedesktop.DBus.ObjectManager'


def dbus2primitive(value):
    """Convert values from dbus types to python types.

    @param value: dbus object to convert to a primitive.

    """
    if isinstance(value, dbus.Boolean):
        return bool(value)
    elif isinstance(value, int):
        return int(value)
    elif isinstance(value, dbus.UInt16):
        return long(value)
    elif isinstance(value, dbus.UInt32):
        return long(value)
    elif isinstance(value, dbus.UInt64):
        return long(value)
    elif isinstance(value, float):
        return float(value)
    elif isinstance(value, str):
        return str(value)
    elif isinstance(value, unicode):
        return str(value)
    elif isinstance(value, list):
        return [dbus2primitive(x) for x in value]
    elif isinstance(value, tuple):
        return tuple([dbus2primitive(x) for x in value])
    elif isinstance(value, dict):
        return dict([(dbus2primitive(k), dbus2primitive(v))
                     for k,v in value.items()])
    else:
        logging.error('Failed to convert dbus object of class: %r',
                      value.__class__.__name__)
        return value


def get_objects_with_interface(service_name, object_manager_path,
                               dbus_interface, path_prefix=None,
                               bus=None):
    """Get objects that have a particular interface via a property manager.

    @param service_name: string remote service exposing the object manager
            to query (e.g. 'org.chromium.peerd').
    @param object_manager_path: string DBus path of object manager on remote
            service (e.g. '/org/chromium/peerd')
    @param dbus_interface: string interface of object we're interested in.
    @param path_prefix: string prefix of DBus path to filter for.  If not
            None, we'll return only objects in the remote service whose
            paths start with this prefix.
    @param bus: dbus.Bus object, defaults to dbus.SystemBus().  Note that
            normally, dbus.SystemBus() multiplexes a single DBus connection
            among its instances.
    @return dict that maps object paths to dicts of interface name to properties
            exposed by that interface.  This is similar to the structure
            returned by org.freedesktop.DBus.ObjectManaber.GetManagedObjects().

    """
    if bus is None:
        bus = dbus.SystemBus()
    object_manager = dbus.Interface(
            bus.get_object(service_name, object_manager_path),
            dbus_interface=DBUS_INTERFACE_OBJECT_MANAGER)
    objects = dbus2primitive(object_manager.GetManagedObjects())
    logging.debug('Saw objects %r', objects)
    # Filter by interface.
    objects = [(path, interfaces)
               for path, interfaces in objects.iteritems()
               if dbus_interface in interfaces]
    if path_prefix is not None:
        objects = [(path, interfaces)
                   for path, interfaces in objects
                   if path.startswith(path_prefix)]
    objects = dict(objects)
    logging.debug('Filtered objects: %r', objects)
    return objects
