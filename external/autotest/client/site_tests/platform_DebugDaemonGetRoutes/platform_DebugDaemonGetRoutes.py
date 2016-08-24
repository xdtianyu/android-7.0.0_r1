# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import dbus
import logging

from autotest_lib.client.bin import test

class platform_DebugDaemonGetRoutes(test.test):
    """Checks that the debugd GetRoutes function is working."""
    version = 1

    def run_once(self, *args, **kwargs):
        bus = dbus.SystemBus()
        proxy = bus.get_object('org.chromium.debugd', '/org/chromium/debugd')
        self.iface = dbus.Interface(proxy,
                                    dbus_interface='org.chromium.debugd')
        ip4_routes = self.iface.GetRoutes({})
        logging.debug('IP4 Routes: %s', ip4_routes)
        ip6_routes = self.iface.GetRoutes({'v6': True})
        logging.debug('IP6 Routes: %s', ip6_routes)
