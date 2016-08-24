# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import dbus
import dbus.service
import logging
import time

from autotest_lib.client.cros.tendo import peerd_dbus_helper
from autotest_lib.client.cros.tendo.n_faced_peerd import dbus_property_exposer
from autotest_lib.client.cros.tendo.n_faced_peerd import service


class Peer(dbus_property_exposer.DBusPropertyExposer):
    """Represents local and remote peers."""

    def __init__(self, bus, path, peer_id, object_manager, is_self=False):
        """Construct a org.chromium.peerd.Peer DBus object.

        @param bus: dbus.Bus object to export this object on.
        @param path: string object path to export this object at.
        @param peer_id: string peerd peer ID; a UUID.
        @param is_self: True iff this object will servce as a self peer.
        @param object_manager: an instance of ObjectManager.

        """
        super(Peer, self).__init__(
                bus, path, peerd_dbus_helper.DBUS_INTERFACE_PEER)
        # Fill in the initial values for our properties.
        self.uuid = peer_id
        self._is_self = is_self
        self._update_last_seen()
        # Register properties with the property exposer.
        self.register_property(peerd_dbus_helper.PEER_PROPERTY_ID,
                               self._get_dbus_id)
        self.register_property(peerd_dbus_helper.PEER_PROPERTY_LAST_SEEN,
                               self._get_dbus_last_seen)
        # Claim our interace with the object manager.
        self._object_manager = object_manager
        self._path = path
        self._object_manager.claim_interface(
                path, peerd_dbus_helper.DBUS_INTERFACE_PEER,
                self.property_getter)
        # We need to keep a good bit of stuff around because we're responsible
        # for creating child service objects.
        self._bus = bus
        self.services = dict()
        self._services_counter = 0


    def _get_dbus_id(self):
        """Getter for PEER_PROPERTY_ID.

        @return dbus.String containing our peer ID.

        """
        return dbus.String(self.uuid)


    def _get_dbus_last_seen(self):
        """Getter for PEER_PROPERTY_LAST_SEEN.

        @return dbus.UInt64 containing the last time this peer was seen
                in milliseconds since the Unix epoc.

        """
        return dbus.UInt64(int(1000 * self._last_seen_seconds))


    def _update_last_seen(self):
        """Updates our last seen value.

        This would be a simple call to time.time(), except that peerd
        has to report a last seen time of 0 for the peer object representing
        itself.

        """
        if self._is_self:
            self._last_seen_seconds = 0
        else:
            self._last_seen_seconds = time.time()


    def close(self):
        """Releases interfaces claimed over DBus."""
        # TODO(wiley) call close on child services.
        raise NotImplementedError('Peer.close() does not work properly')


    def update_service(self, service_id, service_info, ip_info):
        """Update a service associated with this peer.

        @param service_id: string peerd service ID.
        @param service_info: dictionary of string,string items comprising
                the metadata for the service.
        @param ip_info: an instance of IpInfo defined in service.py.

        """
        if service_id in self.services:
            self.services[service_id].update(service_info, ip_info)
        else:
            self._services_counter += 1
            service_path = '%s/services/%d' % (self._path,
                                               self._services_counter)
            self.services[service_id] = service.Service(
                    self._bus, service_path, self.uuid, service_id,
                    service_info, ip_info, self._object_manager)
        logging.info('service=%s has info %r.', service_id, service_info)
        self._update_last_seen()
        self.on_property_changed(peerd_dbus_helper.PEER_PROPERTY_LAST_SEEN)


    def remove_service(self, service_id):
        """Remove a service associated with this peer.

        @param service_id: string peerd service ID.

        """
        removed_service = self.services.pop(service_id, None)
        if removed_service is not None:
            removed_service.close()
        self._update_last_seen()
        self.on_property_changed(peerd_dbus_helper.PEER_PROPERTY_LAST_SEEN)
