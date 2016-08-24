# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import dbus
import time

from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import dbus_util

MANAGER_INTERFACE = 'org.chromium.Buffet.Manager'
SERVICE_NAME = 'org.chromium.Buffet'
MANAGER_OBJECT_PATH = '/org/chromium/Buffet/Manager'
COMMAND_INTERFACE = 'org.chromium.Buffet.Command'
MANAGER_INTERFACE = 'org.chromium.Buffet.Manager'
OBJECT_MANAGER_PATH = '/org/chromium/Buffet'
DBUS_OBJECT_MANAGER_INTERFACE = 'org.freedesktop.DBus.ObjectManager'
DBUS_PROPERTY_INTERFACE = 'org.freedesktop.DBus.Properties'

TEST_MESSAGE = 'Hello world!'

class BuffetDBusHelper(object):
    """Delegate representing an instance of buffet."""

    def __init__(self):
        """Construct a BuffetDBusHelper.

        You should probably use get_helper() above rather than call this
        directly.

        @param manager_proxy: DBus proxy for the Manager object.

        """
        start_time = time.time()
        while time.time() - start_time < 10:
            try:
                self._init()
                if self.manager.TestMethod(TEST_MESSAGE) == TEST_MESSAGE:
                    return
            except:
                pass
            time.sleep(0.5)
        raise error.TestFail('Buffet failed to restart in time.')


    def _init(self):
        """Init members.

        """
        bus = dbus.SystemBus()
        manager_proxy = bus.get_object(SERVICE_NAME, MANAGER_OBJECT_PATH)
        self.manager = dbus.Interface(manager_proxy, MANAGER_INTERFACE)
        self.properties = dbus.Interface(manager_proxy, DBUS_PROPERTY_INTERFACE)
        self.object_manager = dbus.Interface(
                bus.get_object(SERVICE_NAME, OBJECT_MANAGER_PATH),
                dbus_interface=DBUS_OBJECT_MANAGER_INTERFACE)


    def __getattr__(self, name):
        components = name.split('_')
        name = ''.join(x.title() for x in components)
        dbus_value = self.properties.Get(MANAGER_INTERFACE, name)
        return dbus_util.dbus2primitive(dbus_value)
