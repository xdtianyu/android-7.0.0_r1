# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import collections
import dbus.service

from autotest_lib.client.cros.tendo import peerd_dbus_helper
from autotest_lib.client.cros.tendo.n_faced_peerd import dbus_property_exposer

IpInfo = collections.namedtuple('IpInfo', ['addr', 'port'])

class Service(dbus_property_exposer.DBusPropertyExposer):
    """Represents local and remote services."""


    def __init__(self, bus, path, peer_id, service_id, service_info, ip_info,
                 object_manager):
        """Construct a org.chromium.peerd.Service DBus object.

        @param bus: dbus.Bus object to export this object on.
        @param path: string object path to export this object at.
        @param service_id: string peerd service ID.
        @param service_info: dictionary of string,string items comprising
                the metadata for the service.
        @param ip_info: an instance of IpInfo defined above.
        @param object_manager: an instance of ObjectManager.

        """
        super(Service, self).__init__(
                bus, path, peerd_dbus_helper.DBUS_INTERFACE_SERVICE)
        self.peer_id = peer_id
        self.service_id = service_id
        self.service_info = service_info
        self.ip_info = ip_info
        # Register properties with the property exposer.
        self.register_property(peerd_dbus_helper.SERVICE_PROPERTY_PEER_ID,
                               self._get_peer_id)
        self.register_property(peerd_dbus_helper.SERVICE_PROPERTY_ID,
                               self._get_service_id)
        self.register_property(peerd_dbus_helper.SERVICE_PROPERTY_INFO,
                               self._get_service_info)
        self.register_property(peerd_dbus_helper.SERVICE_PROPERTY_IPS,
                               self._get_ip_info)
        # Claim the service interface.
        self._object_manager = object_manager
        self._path = path
        self._object_manager.claim_interface(
                path, peerd_dbus_helper.DBUS_INTERFACE_SERVICE,
                self.property_getter)


    def _get_peer_id(self):
        """Getter for SERVICE_PROPERTY_PEER_ID.

        @return dbus.String containing this service's peer_id.

        """
        return dbus.String(self.peer_id)


    def _get_service_id(self):
        """Getter for SERVICE_PROPERTY_ID.

        @return dbus.String containing this service's service_id.

        """
        return dbus.String(self.service_id)


    def _get_service_info(self):
        """Getter for SERVICE_PROPERTY_INFO.

        @return dbus.Dictionary containing this service's metadata.

        """
        return dbus.Dictionary(self.service_info, 'ss')


    def _get_ip_info(self):
        """Getter for SERVICE_PROPERTY_IPS.

        @return dbus.Array of dbus.Struct objects containing an array of bytes
                and a port number.  See DBus API documentation.

        """
        dbus_port = dbus.UInt16(self.ip_info.port)
        dbus_ip = dbus.Array([dbus.Byte(int(octet))
                              for octet in self.ip_info.addr.split('.')])
        ip_info = dbus.Struct((dbus_ip, dbus_port), signature='ayq')
        return dbus.Array([ip_info], signature='(ayq)')


    def update(self, service_info, ip_info):
        """Update service metadata and trigger DBus signals.

        @param service_info: see constructor.
        @param ip_info: see constructor.

        """
        if service_info != self.service_info:
            self.service_info = service_info
            self.on_property_changed(peerd_dbus_helper.SERVICE_PROPERTY_INFO)
        if ip_info != self.ip_info:
            self.ip_info = ip_info
            self.on_property_changed(peerd_dbus_helper.SERVICE_PROPERTY_IPS)


    def close(self):
        """Release interfaces claimed by this object."""
        self._object_manager.release_interface(
                self._path, peerd_dbus_helper.DBUS_INTERFACE_SERVICE)
