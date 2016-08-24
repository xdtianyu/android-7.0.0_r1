# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import time

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import utils
from autotest_lib.client.common_lib.cros import dbus_send


SERVICE_NAME = 'org.chromium.peerd'

DBUS_PATH_MANAGER = '/org/chromium/peerd/Manager'
DBUS_PATH_SELF = '/org/chromium/peerd/Self'

DBUS_INTERFACE_MANAGER = 'org.chromium.peerd.Manager'
DBUS_INTERFACE_PEER = 'org.chromium.peerd.Peer'
DBUS_INTERFACE_SERVICE = 'org.chromium.peerd.Service'

OBJECT_MANAGER_PATH = '/org/chromium/peerd'

SERVICE_PROPERTY_SERVICE_ID = 'ServiceId'


def confirm_peerd_up(service_name=SERVICE_NAME, timeout_seconds=10, host=None):
    """Confirm that an instance of peerd is running.

    @param service_name: string name of DBus connection to look for peerd on.
            Defaults to the well known peerd bus name.
    @param timeout_seconds: number of seconds to wait for peerd to answer
            queries.
    @param host: Host object if peerd is running on a remote host.

    """
    start_time = time.time()
    while time.time() - start_time < timeout_seconds:
        result = dbus_send.dbus_send(
                service_name, DBUS_INTERFACE_MANAGER, DBUS_PATH_MANAGER,
                'Ping', host=host, tolerate_failures=True)
        if result is not None and result.response == 'Hello world!':
            return
        time.sleep(0.5)
    raise error.TestFail('Timed out before peerd at %s started.' % service_name)


class PeerdConfig(object):
    """An object that knows how to restart peerd in various configurations."""

    def __init__(self, mdns_prefix=None, verbosity_level=None):
        """Construct a peerd configuration.

        @param verbosity_level: int level of log verbosity from peerd (e.g. 0
                                will log INFO level, 3 is verbosity level 3).
        @param mdns_prefix: string prefix for mDNS records.  Will be ignored if
                            using that prefix causes name conflicts.

        """
        self.mdns_prefix = mdns_prefix
        self.verbosity_level = verbosity_level


    def restart_with_config(self, host=None, timeout_seconds=10):
        """Restart peerd with this config.

        @param host: Host object if peerd is running on a remote host.
        @param timeout_seconds: number of seconds to wait for peerd to start.
                Pass None to return without confirming peerd startup.

        """
        run = utils.run if host is None else host.run
        flag_list = []
        if self.verbosity_level is not None:
            flag_list.append('PEERD_LOG_LEVEL=%d' % self.verbosity_level)
        if self.mdns_prefix is not None:
            flag_list.append('PEERD_INITIAL_MDNS_PREFIX=%s' % self.mdns_prefix)
        run('stop peerd', ignore_status=True)
        run('start peerd %s' % ' '.join(flag_list))
        if timeout_seconds is None:
            return
        confirm_peerd_up(service_name=SERVICE_NAME,
                         timeout_seconds=timeout_seconds,
                         host=host)
