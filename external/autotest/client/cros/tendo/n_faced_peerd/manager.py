# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import collections
import dbus
import dbus.bus
import dbus.service
import logging
import uuid


from autotest_lib.client.cros import dbus_util
from autotest_lib.client.cros.tendo import peerd_dbus_helper
from autotest_lib.client.cros.tendo.n_faced_peerd import dbus_property_exposer
from autotest_lib.client.cros.tendo.n_faced_peerd import peer
from autotest_lib.client.cros.tendo.n_faced_peerd import service

# A tuple of a bus name that sent us an ExposeService message, and an
# object responsible for watching for the death of that bus name's
# DBus connection.
SenderWatch = collections.namedtuple('SenderWatch', ['sender', 'watch'])


IGNORED_MONITORING_TOKEN_VALUE = 'This is a monitoring token.'
class InvalidMonitoringTokenException(Exception):
    """Self explanatory."""


class Manager(dbus_property_exposer.DBusPropertyExposer):
    """Represents an instance of org.chromium.peerd.Manager."""

    def __init__(self, bus, ip_address, on_service_modified, unique_name,
                 object_manager):
        """Construct an instance of Manager.

        @param bus: dbus.Bus object to export this object on.
        @param ip_address: string IP address (e.g. '127.0.01').
        @param on_service_modified: callback that takes a Manager instance and
                a service ID.  We'll call this whenever we Expose/Remove a
                service via the DBus API.
        @param unique_name: string DBus well known name to claim on DBus.
        @param object_manager: an instance of ObjectManager.

        """
        super(Manager, self).__init__(bus,
                                      peerd_dbus_helper.DBUS_PATH_MANAGER,
                                      peerd_dbus_helper.DBUS_INTERFACE_MANAGER)
        self._bus = bus
        self._object_manager = object_manager
        self._peer_counter = 0
        self._peers = dict()
        self._ip_address = ip_address
        self._on_service_modified = on_service_modified
        # A map from service_ids to dbus.bus.NameOwnerWatch objects.
        self._watches = dict()
        self.self_peer = peer.Peer(self._bus,
                                   peerd_dbus_helper.DBUS_PATH_SELF,
                                   uuid.uuid4(),
                                   self._object_manager,
                                   is_self=True)
        # TODO(wiley) Expose monitored technologies property
        self._object_manager.claim_interface(
                peerd_dbus_helper.DBUS_PATH_MANAGER,
                peerd_dbus_helper.DBUS_INTERFACE_MANAGER,
                self.property_getter)
        if (self._bus.request_name(unique_name) !=
                dbus.bus.REQUEST_NAME_REPLY_PRIMARY_OWNER):
            raise Exception('Failed to claim name %s' % unique_name)


    def _on_name_owner_changed(self, service_id, owner):
        """Callback that removes a service when the owner disconnects from DBus.

        @param service_id: string service_id of service to remove.
        @param owner: dbus.String identifier of service owner.

        """
        owner = dbus_util.dbus2primitive(owner)
        logging.debug('Name owner for service=%s changed to "%s".',
                      service_id, owner)
        if not owner:
            self.RemoveExposedService(service_id)


    def close(self):
        """Release resources held by this object and child objects."""
        # TODO(wiley) call close on self and remote peers.
        raise NotImplementedError('Manager.close() does not work properly')


    def add_remote_peer(self, remote_peer):
        """Add a remote peer to this object.

        For any given face of NFacedPeerd, the other N - 1 faces are treated
        as "remote peers" that we instantly discover changes on.

        @param remote_peer: Peer object.  Should be the |self_peer| of another
                instance of Manager.

        """
        logging.info('Adding remote peer %s', remote_peer.uuid)
        self._peer_counter += 1
        peer_path = '%s%d' % (peerd_dbus_helper.PEER_PATH_PREFIX,
                              self._peer_counter)
        self._peers[remote_peer.uuid] = peer.Peer(
                self._bus, peer_path, remote_peer.uuid, self._object_manager)


    def on_remote_service_modified(self, remote_peer, service_id):
        """Cause this face to update its view of a remote peer.

        @param remote_peer: Peer object.  Should be the |self_peer| of another
                instance of Manager.
        @param service_id: string service ID of remote service that has changed.
                Note that this service may have been removed.

        """
        local_peer = self._peers[remote_peer.uuid]
        remote_service = remote_peer.services.get(service_id)
        if remote_service is not None:
            logging.info('Exposing remote service: %s', service_id)
            local_peer.update_service(remote_service.service_id,
                                      remote_service.service_info,
                                      remote_service.ip_info)
        else:
            logging.info('Removing remote service: %s', service_id)
            local_peer.remove_service(service_id)


    @dbus.service.method(
            dbus_interface=peerd_dbus_helper.DBUS_INTERFACE_MANAGER,
            in_signature='sa{ss}a{sv}', out_signature='',
            sender_keyword='sender')
    def ExposeService(self, service_id, service_info, options, sender=None):
        """Implementation of org.chromium.peerd.Manager.ExposeService().

        @param service_id: see DBus API documentation.
        @param service_info: see DBus API documentation.
        @param options: see DBus API documentation.
        @param sender: string DBus connection ID of caller.  Must match
                |sender_keyword| value in decorator.

        """
        if (service_id in self._watches and
                sender != self._watches[service_id].sender):
            logging.error('Asked to advertise a duplicate service by %s. '
                          'Expected %s.',
                          sender, self._watches[service_id].sender)
            raise Exception('Will not advertise duplicate services.')
        logging.info('Exposing service %s', service_id)
        port = options.get('mdns', dict()).get('port', 0)
        self.self_peer.update_service(
                service_id, service_info,
                service.IpInfo(addr=self._ip_address, port=port))
        if service_id not in self._watches:
            cb = lambda owner: self._on_name_owner_changed(service_id, owner)
            watch = dbus.bus.NameOwnerWatch(self._bus, sender, cb)
            self._watches[service_id] = SenderWatch(sender, watch)
        self._on_service_modified(self, service_id)


    @dbus.service.method(
            dbus_interface=peerd_dbus_helper.DBUS_INTERFACE_MANAGER,
            in_signature='s', out_signature='')
    def RemoveExposedService(self, service_id):
        """Implementation of org.chromium.peerd.Manager.RemoveExposedService().

        @param service_id: see DBus API documentation.

        """
        logging.info('Removing service %s', service_id)
        self._watches.pop(service_id, None)
        self.self_peer.remove_service(service_id)
        self._on_service_modified(self, service_id)


    @dbus.service.method(
            dbus_interface=peerd_dbus_helper.DBUS_INTERFACE_MANAGER,
            in_signature='asa{sv}', out_signature='s')
    def StartMonitoring(self, technologies, options):
        """Fake implementation of org.chromium.peerd.Manager.StartMonitoring().

        We do not monitor anything in NFacedPeerd.

        @param technologies: See DBus API documentation.
        @param options: See DBus API documentation.

        """
        return IGNORED_MONITORING_TOKEN_VALUE


    @dbus.service.method(
            dbus_interface=peerd_dbus_helper.DBUS_INTERFACE_MANAGER,
            in_signature='s', out_signature='')
    def StopMonitoring(self, monitoring_token):
        """Fake implementation of org.chromium.peerd.Manager.StopMonitoring().

        We do not monitor anything in NFacedPeerd

        @param monitoring_token: See DBus API documentation.

        """
        if monitoring_token != IGNORED_MONITORING_TOKEN_VALUE:
            raise InvalidMonitoringTokenException()


    @dbus.service.method(
            dbus_interface=peerd_dbus_helper.DBUS_INTERFACE_MANAGER,
            in_signature='', out_signature='s')
    def Ping(self):
        """Implementation of org.chromium.peerd.Manager.Ping()."""
        return 'Hello world!'
