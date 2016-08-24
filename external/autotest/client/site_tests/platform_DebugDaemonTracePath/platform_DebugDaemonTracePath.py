# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import dbus

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error

class platform_DebugDaemonTracePath(test.test):
    version = 1

    def run_once(self, *args, **kwargs):
        bus = dbus.SystemBus()
        proxy = bus.get_object('org.chromium.debugd', '/org/chromium/debugd')
        self.iface = dbus.Interface(proxy,
                                    dbus_interface='org.chromium.debugd')
        handle = self.iface.TracePathStart(1, "127.0.0.1", {})
        self.iface.TracePathStop(handle)
        got_exception = False
        try:
            self.iface.TracePathStop(handle)
        except dbus.DBusException as e:
            if e.get_dbus_name() == 'org.chromium.debugd.error.NoSuchProcess':
                got_exception = True
            else:
                print "Unexpected exception %s" % e.get_dbus_name()
        if not got_exception:
            raise error.TestFail("Didn't get expected exception.")
