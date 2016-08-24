# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import dbus

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error


SERVICE_NAME = 'org.chromium.WebServer'
MANAGER_INTERFACE = 'org.chromium.WebServer.Manager'
MANAGER_OBJECT_PATH = '/org/chromium/WebServer/Manager'

EXPECTED_PING_RESPONSE = 'Web Server is running'

class webservd_BasicDBusAPI(test.test):
    """Check that basic webservd daemon DBus APIs are functional."""
    version = 1

    def run_once(self):
        """Test entry point."""
        bus = dbus.SystemBus()
        manager_proxy = dbus.Interface(
                bus.get_object(SERVICE_NAME, MANAGER_OBJECT_PATH),
                dbus_interface=MANAGER_INTERFACE)
        ping_response = manager_proxy.Ping()
        if EXPECTED_PING_RESPONSE != ping_response:
            raise error.TestFail(
                    'Expected Manager.Ping to return %s but got %s instead.' %
                    (EXPECTED_PING_RESPONSE, ping_response))
