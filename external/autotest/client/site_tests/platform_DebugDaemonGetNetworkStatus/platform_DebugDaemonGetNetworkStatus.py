# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import dbus
import json
import logging

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error

class platform_DebugDaemonGetNetworkStatus(test.test):
    version = 1

    def run_once(self, *args, **kwargs):
        bus = dbus.SystemBus()
        proxy = bus.get_object('org.chromium.debugd', '/org/chromium/debugd')
        self.iface = dbus.Interface(proxy,
                                    dbus_interface='org.chromium.debugd')
        result = self.iface.GetNetworkStatus()
        logging.info('Result: %s' % result)
        networks = json.loads(result)
        if 'services' not in networks or 'devices' not in networks:
            raise error.TestFail('No networks found: %s' % result)
