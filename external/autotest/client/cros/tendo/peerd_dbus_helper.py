# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import collections
import dbus
import dbus.mainloop.glib
import logging
import time

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import utils
from autotest_lib.client.cros import dbus_util

Service = collections.namedtuple('Service',
                                 ['service_id', 'service_info', 'service_ips'])
Peer = collections.namedtuple('Peer', ['uuid', 'last_seen', 'services'])

# DBus constants for use with peerd.
SERVICE_NAME = 'org.chromium.peerd'
DBUS_INTERFACE_MANAGER = 'org.chromium.peerd.Manager'
DBUS_INTERFACE_PEER = 'org.chromium.peerd.Peer'
DBUS_INTERFACE_SERVICE = 'org.chromium.peerd.Service'
DBUS_INTERFACE_OBJECT_MANAGER = 'org.freedesktop.DBus.ObjectManager'
DBUS_PATH_MANAGER = '/org/chromium/peerd/Manager'
DBUS_PATH_OBJECT_MANAGER = '/org/chromium/peerd'
DBUS_PATH_SELF = '/org/chromium/peerd/Self'
PEER_PATH_PREFIX = '/org/chromium/peerd/peers/'
PEER_PROPERTY_ID = 'UUID'
PEER_PROPERTY_LAST_SEEN = 'LastSeen'
SERVICE_PROPERTY_ID = 'ServiceId'
SERVICE_PROPERTY_INFO = 'ServiceInfo'
SERVICE_PROPERTY_IPS = 'IpInfos'
SERVICE_PROPERTY_PEER_ID = 'PeerId'

# Possible technologies for use with PeerdDBusHelper.start_monitoring().
TECHNOLOGY_ALL = 'all'
TECHNOLOGY_MDNS = 'mDNS'

# We can give some options to ExposeService.
EXPOSE_SERVICE_SECTION_MDNS = 'mdns'
EXPOSE_SERVICE_MDNS_PORT = 'port'

def make_helper(peerd_config, bus=None, timeout_seconds=10):
    """Wait for peerd to come up, then return a PeerdDBusHelper for it.

    @param peerd_config: a PeerdConfig object.
    @param bus: DBus bus to use, or specify None to create one internally.
    @param timeout_seconds: number of seconds to wait for peerd to come up.
    @return PeerdDBusHelper instance if peerd comes up, None otherwise.

    """
    start_time = time.time()
    peerd_config.restart_with_config(timeout_seconds=timeout_seconds)
    if bus is None:
        dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)
        bus = dbus.SystemBus()
    while time.time() - start_time < timeout_seconds:
        if not bus.name_has_owner(SERVICE_NAME):
            time.sleep(0.2)
        return PeerdDBusHelper(bus)
    raise error.TestFail('peerd did not start in a timely manner.')


class PeerdDBusHelper(object):
    """Container for convenience methods related to peerd."""

    def __init__(self, bus):
        """Construct a PeerdDBusHelper.

        @param bus: DBus bus to use, or specify None and this object will
                    create a mainloop and bus.

        """
        self._bus = bus
        self._manager = dbus.Interface(
                self._bus.get_object(SERVICE_NAME, DBUS_PATH_MANAGER),
                DBUS_INTERFACE_MANAGER)


    def _get_peers(self):
        object_manager = dbus.Interface(
                self._bus.get_object(SERVICE_NAME, DBUS_PATH_OBJECT_MANAGER),
                DBUS_INTERFACE_OBJECT_MANAGER)
        # |dbus_objects| is a map<object path,
        #                         map<interface name,
        #                             map<property name, value>>>
        dbus_objects = object_manager.GetManagedObjects()
        objects = dbus_util.dbus2primitive(dbus_objects)
        peer_objects = [(path, interfaces)
                        for path, interfaces in objects.iteritems()
                        if (path.startswith(PEER_PATH_PREFIX) and
                            DBUS_INTERFACE_PEER in interfaces)]
        peers = []
        for peer_path, interfaces in peer_objects:
            service_property_sets = [
                    interfaces[DBUS_INTERFACE_SERVICE]
                    for path, interfaces in objects.iteritems()
                    if (path.startswith(peer_path + '/services/') and
                        DBUS_INTERFACE_SERVICE in interfaces)]
            services = []
            for service_properties in service_property_sets:
                logging.debug('Found service with properties: %r',
                              service_properties)
                ip_addrs = [('.'.join(map(str, ip)), port) for ip, port
                            in service_properties[SERVICE_PROPERTY_IPS]]
                services.append(Service(
                        service_id=service_properties[SERVICE_PROPERTY_ID],
                        service_info=service_properties[SERVICE_PROPERTY_INFO],
                        service_ips=ip_addrs))
            peer_properties = interfaces[DBUS_INTERFACE_PEER]
            peer = Peer(uuid=peer_properties[PEER_PROPERTY_ID],
                        last_seen=peer_properties[PEER_PROPERTY_LAST_SEEN],
                        services=services)
            peers.append(peer)
        return peers


    def close(self):
        """Clean up peerd state related to this helper."""
        utils.run('stop peerd')
        utils.run('start peerd')


    def start_monitoring(self, technologies):
        """Monitor the specified technologies.

        Note that peerd will watch bus connections and stop monitoring a
        technology if this bus connection goes away.A

        @param technologies: iterable container of TECHNOLOGY_* defined above.
        @return string monitoring_token for use with stop_monitoring().

        """
        return self._manager.StartMonitoring(technologies,
                                             dbus.Dictionary(signature='sv'))


    def has_peer(self, uuid):
        """
        Return a Peer instance if peerd has found a matching peer.

        Optional parameters are also matched if not None.

        @param uuid: string unique identifier of peer.
        @return Peer tuple if a matching peer exists, None otherwise.

        """
        peers = self._get_peers()
        logging.debug('Found peers: %r.', peers)
        for peer in peers:
            if peer.uuid != uuid:
                continue
            return peer
        logging.debug('No peer had a matching ID.')
        return None


    def expose_service(self, service_id, service_info, mdns_options=None):
        """Expose a service via peerd.

        Note that peerd should watch DBus connections and remove this service
        if our bus connection ever goes down.

        @param service_id: string id of service.  See peerd documentation
                           for limitations on this string.
        @param service_info: dict of string, string entries.  See peerd
                             documentation for relevant restrictions.
        @param mdns_options: dict of string, <variant type>.
        @return string service token for use with remove_service().

        """
        options = dbus.Dictionary(signature='sv')
        if mdns_options is not None:
            options[EXPOSE_SERVICE_SECTION_MDNS] = dbus.Dictionary(
                    signature='sv')
            # We're going to do a little work here to make calling us easier.
            for k,v in mdns_options.iteritems():
                if k == EXPOSE_SERVICE_MDNS_PORT:
                    v = dbus.UInt16(v)
                options[EXPOSE_SERVICE_SECTION_MDNS][k] = v
        self._manager.ExposeService(service_id, service_info, options)


    def remove_service(self, service_id):
        """Remove a service previously added via expose_service().

        @param service_id: string service ID of service to remove.

        """
        self._manager.RemoveExposedService(service_id)
