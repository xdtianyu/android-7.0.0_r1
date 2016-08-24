# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import dbus
import logging
import time

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.networking import shill_context
from autotest_lib.client.cros.networking import shill_proxy


class network_3GStressEnable(test.test):
    """
    Stress-tests enabling and disabling a technology at short intervals.

    """
    version = 1

    okerrors = [
        shill_proxy.ShillProxy.ERROR_IN_PROGRESS
    ]

    def _enable_device(self, enable):
        try:
            timeout = shill_proxy.ShillProxy.DEVICE_ENABLE_DISABLE_TIMEOUT
            if enable:
                self.device.Enable(timeout=timeout)
            else:
                self.device.Disable(timeout=timeout)
        except dbus.exceptions.DBusException, err:
            if err.get_dbus_name() in network_3GStressEnable.okerrors:
                return
            raise error.TestFail(err)


    def _test(self, settle):
        self._enable_device(False)
        time.sleep(settle)
        self._enable_device(True)
        time.sleep(settle)


    def run_once(self, test_env, cycles=3, min=15, max=25):
        with test_env, shill_context.ServiceAutoConnectContext(
                test_env.shill.find_cellular_service_object, False):
            self.device = test_env.shill.find_cellular_device_object()
            for t in xrange(max, min, -1):
                for n in xrange(cycles):
                    # deciseconds are an awesome unit.
                    logging.info('Cycle %d: %f seconds delay.', n, t / 10.0)
                    self._test(t / 10.0)
            logging.info('Done.')
