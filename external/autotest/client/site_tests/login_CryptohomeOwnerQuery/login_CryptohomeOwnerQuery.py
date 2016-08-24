# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Most of this code is based on login_GuestAndActualSession, which performs
# similar ownership clearing/checking tasks.

import gobject, os
from dbus.mainloop.glib import DBusGMainLoop

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import session_manager
from autotest_lib.client.cros import cros_ui, cryptohome, ownership

class login_CryptohomeOwnerQuery(test.test):
    """Verify that the cryptohome owner user query works properly."""
    version = 1


    def initialize(self):
        super(login_CryptohomeOwnerQuery, self).initialize()
        # Ensure a clean beginning.
        ownership.restart_ui_to_clear_ownership_files()

        bus_loop = DBusGMainLoop(set_as_default=True)
        self._session_manager = session_manager.connect(bus_loop)
        self._listener = session_manager.OwnershipSignalListener(
                gobject.MainLoop())
        self._listener.listen_for_new_key_and_policy()

        self._cryptohome_proxy = cryptohome.CryptohomeProxy(bus_loop)


    def run_once(self):
        owner = 'first_user@nowhere.com'

        if cryptohome.get_login_status()['owner_user_exists']:
            raise error.TestFail('Owner existed before login')

        self._cryptohome_proxy.ensure_clean_cryptohome_for(owner)
        if not self._session_manager.StartSession(owner, ''):
            raise error.TestFail('Could not start session for ' + owner)

        self._listener.wait_for_signals(desc='Device ownership complete.')

        if not cryptohome.get_login_status()['owner_user_exists']:
            raise error.TestFail('Owner does not exist after login')


    def cleanup(self):
        self._session_manager.StopSession('')
        cros_ui.start(allow_fail=True, wait_for_login_prompt=False)
