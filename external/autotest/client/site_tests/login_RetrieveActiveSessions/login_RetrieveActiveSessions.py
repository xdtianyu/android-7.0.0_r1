# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from dbus.mainloop.glib import DBusGMainLoop

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import session_manager
from autotest_lib.client.cros import cros_ui, cryptohome


class login_RetrieveActiveSessions(test.test):
    """Ensure that the session_manager correctly tracks active sessions.
    """
    version = 1


    def initialize(self):
        super(login_RetrieveActiveSessions, self).initialize()
        cros_ui.restart()


    def run_once(self):
        bus_loop = DBusGMainLoop(set_as_default=True)
        sm = session_manager.connect(bus_loop)

        cryptohome_proxy = cryptohome.CryptohomeProxy(bus_loop)
        users = ['first_user@nowhere.com', 'second_user@nowhere.com']
        for user in users:
            cryptohome_proxy.ensure_clean_cryptohome_for(user)

        if not sm.StartSession(users[0], ''):
            raise error.TestError('Could not start session for ' + users[0])
        self.__check_for_users_in_actives(users[:1],
                                          sm.RetrieveActiveSessions())

        if not sm.StartSession(users[1], ''):
            raise error.TestError('Could not start session for ' + users[1])
        self.__check_for_users_in_actives(users, sm.RetrieveActiveSessions())


    def __check_for_users_in_actives(self, users, actives):
        """Checks that only members of users are in actives.

        If there are too many (or too few) entries in actives, this method
        raises.  Also, if each member of users is not present in the keys of
        actives, then the method also raises.

        @param users: iterable of user names to be checked for.
        @param actives: a dictionary of {user: userhash}, the keys of which
                        are expected to match users.

        @raises error.TestFail: if one of the above criteria is not met.
        """
        expected_sessions = len(users)
        if len(actives) != expected_sessions:
            raise error.TestFail("%d session(s) should be active, not: %s" %
                                 (expected_sessions, str(actives)))

        if set(users) != set(actives.keys()):
            raise error.TestFail("Expected sessions for %s, got %s" %
                                 (users, actives))


    def cleanup(self):
        # Bounce UI, without waiting for the browser to come back. Best effort.
        cros_ui.stop(allow_fail=True)
        cros_ui.start(allow_fail=True, wait_for_login_prompt=False)
