# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import dbus
import time

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros.tendo import peerd_config
from autotest_lib.client.cros import dbus_util


SERVICE_ID = 'test-service'

class peerd_MonitorsDBusConnections(test.test):
    """Test that peerd removes services when processes disconnect from DBus."""

    version = 1


    def _check_has_test_service(self, expect_service=True):
        services = dbus_util.get_objects_with_interface(
                peerd_config.SERVICE_NAME,
                peerd_config.OBJECT_MANAGER_PATH,
                peerd_config.DBUS_INTERFACE_SERVICE,
                path_prefix=peerd_config.DBUS_PATH_SELF,
                bus=self._bus)
        found_service = False
        # services is a map of object path to dicts of DBus interface to
        # properties exposed by that interface.
        for path, interfaces in services.iteritems():
            for interface, properties in interfaces.iteritems():
                if interface != peerd_config.DBUS_INTERFACE_SERVICE:
                    continue
                if (properties[peerd_config.SERVICE_PROPERTY_SERVICE_ID]
                        != SERVICE_ID):
                    continue
                if found_service:
                    raise error.TestFail('Found multiple test service '
                                         'instances?')
                found_service = True

        if expect_service != found_service:
            raise error.TestFail('Expected to see test service, but did not.')


    def run_once(self):
        self._bus = dbus.SystemBus()
        config = peerd_config.PeerdConfig(verbosity_level=5)
        config.restart_with_config()
        self._check_has_test_service(expect_service=False)
        self._manager = dbus.Interface(
                self._bus.get_object(peerd_config.SERVICE_NAME,
                                     peerd_config.DBUS_PATH_MANAGER),
                peerd_config.DBUS_INTERFACE_MANAGER)
        self._manager.ExposeService(SERVICE_ID,
                                    dbus.Dictionary(signature='ss'),
                                    dbus.Dictionary(signature='sv'))
        # Python keeps the DBus connection sitting around unless we
        # explicitly close it.  The service should still be there.
        time.sleep(1)  # Peerd might take some time to publish the service.
        self._check_has_test_service()
        # Close our previous connection, open a new one.
        self._bus.close()
        self._bus = dbus.SystemBus()
        time.sleep(1)  # Peerd might take some time to remove the service.
        self._check_has_test_service(expect_service=False)
