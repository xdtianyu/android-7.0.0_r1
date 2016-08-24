# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import dbus
import logging
import random
import time

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.networking import cellular_proxy
from autotest_lib.client.cros.networking import shill_context
from autotest_lib.client.cros.networking import shill_proxy


class network_3GSafetyDance(test.test):
    """
    Stress tests all connection manager 3G operations.

    This test runs a long series of 3G operations in pseudorandom order. All of
    these 3G operations must return a convincing result (EINPROGRESS or no
    error).

    """
    version = 1

    def _filterexns(self, fn):
        v = None
        try:
            v = fn()
        except dbus.exceptions.DBusException, error:
            if error.get_dbus_name() in self.okerrors:
                return v, error.get_dbus_message()
            else:
                raise error
        return v, ''

    def _enable(self):
        logging.info('Enable')
        self._filterexns(lambda:
            self.test_env.shill.manager.EnableTechnology('cellular'))

    def _disable(self):
        logging.info('Disable')
        self._filterexns(lambda:
            self.test_env.shill.manager.DisableTechnology('cellular'))

    def _ignoring(self, reason):
        if ('AlreadyConnected' in reason or
            'Not connected' in reason or
            'Bearer already being connected' in reason or
            'Bearer already being disconnected' in reason or
            'InProgress' in reason):
            return True
        if 'NotSupported' in reason:
            # We should only ignore this error if we've previously disabled
            # cellular technology and the service subsequently disappeared
            # when we tried to connect again.
            return not self.test_env.shill.find_cellular_service_object()
        return False

    def _connect(self):
        logging.info('Connect')
        try:
            service = self.test_env.shill.wait_for_cellular_service_object(
                    timeout_seconds=5)
        except shill_proxy.ShillProxyError:
            return

        success, reason = self._filterexns(lambda:
                self.test_env.shill.connect_service_synchronous(
                        service=service,
                        timeout_seconds=
                        cellular_proxy.CellularProxy.SERVICE_CONNECT_TIMEOUT))
        if not success and not self._ignoring(reason):
            raise error.TestFail('Could not connect: %s' % reason)

    def _disconnect(self):
        logging.info('Disconnect')
        try:
            service = self.test_env.shill.wait_for_cellular_service_object(
                    timeout_seconds=5)
        except shill_proxy.ShillProxyError:
            return

        success, reason = self._filterexns(lambda:
                self.test_env.shill.disconnect_service_synchronous(
                        service=service,
                        timeout_seconds=
                        cellular_proxy.CellularProxy.
                        SERVICE_DISCONNECT_TIMEOUT))
        if not success and not self._ignoring(reason):
            raise error.TestFail('Could not disconnect: %s' % reason)

    def _op(self):
        n = random.randint(0, len(self.ops) - 1)
        self.ops[n]()
        time.sleep(random.randint(5, 20) / 10.0)

    def _run_once_internal(self, ops=30, seed=None):
        if not seed:
            seed = int(time.time())
        self.okerrors = [
            'org.chromium.flimflam.Error.InProgress',
            'org.chromium.flimflam.Error.AlreadyConnected',
            'org.chromium.flimflam.Error.AlreadyEnabled',
            'org.chromium.flimflam.Error.AlreadyDisabled'
        ]
        self.ops = [ self._enable,
                     self._disable,
                     self._connect,
                     self._disconnect ]
        self.device = self.test_env.shill.find_cellular_device_object()
        if not self.device:
            raise error.TestFail('Could not find cellular device.')

        # Start in a disabled state.
        self._disable()
        logging.info('Seed: %d', seed)
        random.seed(seed)
        for _ in xrange(ops):
            self._op()

    def run_once(self, test_env, ops=30, seed=None):
        self.test_env = test_env
        with test_env, shill_context.ServiceAutoConnectContext(
                test_env.shill.find_cellular_service_object, False):
            self._run_once_internal(ops, seed)

            # Enable device to restore autoconnect settings.
            self._enable()
            test_env.shill.wait_for_cellular_service_object()
