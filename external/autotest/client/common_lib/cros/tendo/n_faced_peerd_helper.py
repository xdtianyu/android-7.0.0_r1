# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.client.common_lib.cros import dbus_send
from autotest_lib.client.common_lib.cros import process_watcher
from autotest_lib.client.common_lib.cros.tendo import peerd_config


N_FACED_PEERD_PATH = \
        '/usr/local/autotest/cros/tendo/n_faced_peerd/n_faced_peerd_main.py'
# In test images, we add a supplementary set of rules that expand the DBus
# access policy to tolerate us claiming this name and sending messages to
# these services.
TEST_SERVICE_NAME_PREFIX = 'org.chromium.peerd.test'


def get_nth_service_name(n):
    """Get the DBus service name for the Nth face of peerd.

    @param n: int starting from 0 inclusive.
    @return DBus service name for Nth instance as a string.

    """
    return '%s.TestInstance%d' % (TEST_SERVICE_NAME_PREFIX, n)


class NFacedPeerdHelper(object):
    """Helper object that knows how to start and stop NFacedPeerd."""

    def __init__(self, num_instances, localhost_addr='127.0.0.1', host=None):
        """Construct an NFacedPeerd instance (possibly on a remote host).

        @param num_instances: int number of faces to include in our fake peerd.
        @param localhost_addr: string ip address (e.g. '127.0.0.1').  Each face
                of peerd will be "discovered" by the other faces at this IP
                address.  This should be an IP address that makes sense for
                daemons consuming it.
        @param host: host object if we should start NFacedPeerd on a remote
                host.

        """
        self._process_watcher = process_watcher.ProcessWatcher(
                N_FACED_PEERD_PATH,
                args=['%d' % num_instances,
                      '127.0.0.1'],
                host=host)
        self._num_instances = num_instances
        self._host = host


    def get_face_identifier(self, instance_number):
        """Get the UUID for a given face of NFacedPeerd.

        This will fail if we have not previously started an instance of
        NFacedPeerd.

        @param instance_number: int starting from 0 inclusive.
        @return string UUID value.

        """
        return dbus_send.get_property(
                get_nth_service_name(instance_number),
                peerd_config.DBUS_INTERFACE_PEER,
                peerd_config.DBUS_PATH_SELF,
                'UUID',
                host=self._host).response


    def start(self):
        """Start an instance of NFacedPeerd on the host."""
        logging.debug('Starting NFacedPeerd')
        self._process_watcher.start()
        for i in range(self._num_instances):
            service_name = get_nth_service_name(i)
            peerd_config.confirm_peerd_up(service_name=service_name,
                                          host=self._host)


    def close(self):
        """Close all resources held by the helper."""
        logging.debug('Stopping NFacedPeerd')
        self._process_watcher.close()
        logging.debug('Finished stopping NFacedPeerd')
