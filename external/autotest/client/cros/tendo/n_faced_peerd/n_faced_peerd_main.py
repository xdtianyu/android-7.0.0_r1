#!/usr/bin/python

# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import argparse
import dbus
import dbus.mainloop.glib
import gobject
import logging
import logging.handlers


import common
from autotest_lib.client.common_lib.cros.tendo import n_faced_peerd_helper
from autotest_lib.client.cros.tendo import peerd_dbus_helper
from autotest_lib.client.cros.tendo.n_faced_peerd import manager
from autotest_lib.client.cros.tendo.n_faced_peerd import object_manager


class NFacedPeerd(object):
    """An object which looks like N different instances of peerd.

    There are situations where we would like to run N instances
    of a service on the same system (e.g. testing behavior of
    N instances of leaderd). If that service has a dependency
    on peerd (i.e. it advertises a service), then those N instances
    will conflict on their shared dependency on peerd.

    NFacedPeerd solves this by starting N instances of peerd running
    inside the same process.  Services exposed on a particular face
    are advertised as remote services on the other faces.

    """

    def __init__(self, num_instances, ip_address):
        """Construct an instance.

        @param num_instance: int number of "instances" of peerd to start.
        @param ip_address: string IP address to use in service records for
                all faces.  This should usually be the address of the loopback
                interface.

        """
        self._instances = []
        # This is a class that wraps around a global singleton to provide
        # dbus-python specific functionality.  This design pattern fills
        # me with quiet horror.
        loop = dbus.mainloop.glib.DBusGMainLoop()
        # Construct N fake instances of peerd
        for i in range(num_instances):
            bus = dbus.SystemBus(private=True, mainloop=loop)
            unique_name = n_faced_peerd_helper.get_nth_service_name(i)
            om = object_manager.ObjectManager(
                    bus, peerd_dbus_helper.DBUS_PATH_OBJECT_MANAGER)
            self._instances.append(manager.Manager(
                    bus, ip_address, self._on_service_modified, unique_name, om))
        # Now tell them all about each other
        for instance in self._instances:
            for other_instance in self._instances:
                # Don't tell anyone about themselves, that would be silly.
                if instance == other_instance:
                    continue
                instance.add_remote_peer(other_instance.self_peer)


    def _on_service_modified(self, updated_manager, service_id):
        """Called on a service being modified by a manager.

        We use this callback to propagate services exposed to a particular
        instance of peerd to all other instances of peerd as a remote
        service.  Note that |service_id| could have just been deleted,
        in which case, the lookup for the service will fail.

        @param updated_manager_index: integer index of manager modifying
                the service.
        @param service_id: string service ID of service being modified.

        """
        logging.debug('Service %s modified on instance %r',
                      service_id, updated_manager)
        updated_peer = updated_manager.self_peer
        for other_manager in self._instances:
            if other_manager == updated_manager:
                continue
            other_manager.on_remote_service_modified(updated_peer, service_id)


    def run(self):
        """Enter the mainloop and respond to DBus queries."""
        # Despite going by two different names, this is actually the same
        # mainloop we referenced earlier. Yay!
        loop = gobject.MainLoop()
        loop.run()


def main():
    """Entry point for this daemon."""
    formatter = logging.Formatter(
            'n_faced_peerd: [%(levelname)s] %(message)s')
    handler = logging.handlers.SysLogHandler(address='/dev/log')
    handler.setFormatter(formatter)
    logger = logging.getLogger()
    logger.addHandler(handler)
    logger.setLevel(logging.DEBUG)
    logging.info('NFacedPeerd daemon starting.')
    parser = argparse.ArgumentParser(
        description='Acts like N instances of peerd.')
    parser.add_argument('num_instances', metavar='N', type=int,
                        help='Number of fake instances to start.')
    parser.add_argument(
        'ip_address', metavar='ip_address', type=str,
        help='IP address to claim for all instances (e.g. "127.0.0.1").')
    args = parser.parse_args()
    n_faces = NFacedPeerd(args.num_instances, args.ip_address)
    n_faces.run()
    logging.info('NFacedPeerd daemon mainloop has exitted.')


if __name__ == '__main__':
    main()
