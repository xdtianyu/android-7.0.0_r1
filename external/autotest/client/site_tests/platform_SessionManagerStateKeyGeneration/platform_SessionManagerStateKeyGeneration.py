# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from dbus.mainloop.glib import DBusGMainLoop

from autotest_lib.client.common_lib.cros import session_manager
from autotest_lib.client.common_lib import error
from autotest_lib.client.bin import test, utils
from autotest_lib.client.cros import cros_ui

class platform_SessionManagerStateKeyGeneration(test.test):
    '''Verifies that session_manager's GetServerBackedStateKeys DBus method
    returns valid state keys.'''
    version = 1

    def initialize(self):
        super(platform_SessionManagerStateKeyGeneration, self).initialize()
        cros_ui.stop(allow_fail=True)
        cros_ui.start()
        self._bus_loop = DBusGMainLoop(set_as_default=True)

    def run_once(self):
        try:
            if utils.system_output('crossystem mainfw_type') == 'nonchrome':
                raise error.TestNAError(
                    'State key generation only works on Chrome OS hardware')
        except error.CmdError, e:
            raise error.TestError('Failed to run crossystem: %s' % e)

        # Retrieve state keys.
        session_manager_proxy = session_manager.connect(self._bus_loop)
        state_keys = session_manager_proxy.GetServerBackedStateKeys(
            byte_arrays=True)

        # Sanity-check the state keys.
        if len(state_keys) < 3:
            raise error.TestFail("Not enough state keys")
        if len(state_keys) != len(set(state_keys)):
            raise error.TestFail("Duplicate state keys")
        for state_key in state_keys:
            if len(state_key) != 32:
                raise error.TestFail("Bad state key size")
