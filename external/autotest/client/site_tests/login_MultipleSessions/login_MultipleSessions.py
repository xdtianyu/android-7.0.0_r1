# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import gobject, os
from dbus.mainloop.glib import DBusGMainLoop

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import policy, session_manager
from autotest_lib.client.cros import cros_ui, cryptohome, ownership


class login_MultipleSessions(test.test):
    """Ensure that the session_manager can handle multiple calls to StartSession
       correctly.
    """
    version = 1

    def setup(self):
        os.chdir(self.srcdir)
        utils.make('OUT_DIR=.')


    def initialize(self):
        super(login_MultipleSessions, self).initialize()
        # Ensure a clean beginning.
        ownership.restart_ui_to_clear_ownership_files()

        self._bus_loop = DBusGMainLoop(set_as_default=True)
        self._session_manager = session_manager.connect(self._bus_loop)
        self._listener = session_manager.OwnershipSignalListener(
                gobject.MainLoop())
        self._listener.listen_for_new_key_and_policy()

        self._cryptohome_proxy = cryptohome.CryptohomeProxy(self._bus_loop)


    def run_once(self):
        expected_owner = 'first_user@nowhere.com'
        other_user = 'second_user@nowhere.com'
        self.__start_session_for(expected_owner)
        self.__start_session_for(other_user)
        self._listener.wait_for_signals(desc='Initial policy push complete.')

        # Ensure that the first user got to be the owner.
        retrieved_policy = policy.get_policy(self._session_manager)
        if retrieved_policy is None: raise error.TestFail('Policy not found')
        policy.compare_policy_response(self.srcdir, retrieved_policy,
                                       owner=expected_owner)
        # bounce the session manager and wait for it to come back up before
        # reconnecting.
        cros_ui.restart()
        self._session_manager = session_manager.connect(self._bus_loop)

        # Destroy the owner's cryptohome and start sessions again in a
        # different order
        self.__start_session_for(other_user)
        self.__start_session_for(expected_owner)

        self._listener.wait_for_signals(desc='Re-taking of ownership complete.')

        # Ensure that the first user still gets to be the owner.
        retrieved_policy = policy.get_policy(self._session_manager)
        if retrieved_policy is None: raise error.TestFail('Policy not found')
        policy.compare_policy_response(self.srcdir, retrieved_policy,
                                       owner=expected_owner)


    def __start_session_for(self, user):
        """Call StartSession() for user, ensure he has clean on-device state

        Make a fresh cryptohome for user, and then start a session for him
        with the session manager.

        @param user: the user to start a session for.

        @raises error.TestFail: if the session cannot be started.
        """
        self._cryptohome_proxy.ensure_clean_cryptohome_for(user)
        if not self._session_manager.StartSession(user, ''):
            raise error.TestFail('Could not start session for ' + user)


    def cleanup(self):
        # Bounce UI, without waiting for the browser to come back. Best effort.
        cros_ui.stop(allow_fail=True)
        cros_ui.start(allow_fail=True, wait_for_login_prompt=False)
