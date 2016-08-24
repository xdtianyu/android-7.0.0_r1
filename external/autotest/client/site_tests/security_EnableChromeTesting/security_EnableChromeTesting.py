# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import dbus, logging, os, pwd

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import constants

class security_EnableChromeTesting(test.test):
    version = 1

    def _set_user(self, username):
        user_info = pwd.getpwnam(username)
        os.setegid(user_info[3])
        os.seteuid(user_info[2])
        self._set_user_environment(username)

    def _reset_user(self):
        uid = os.getuid()
        username = pwd.getpwuid(uid)[0]
        os.seteuid(uid)
        os.setegid(os.getgid())
        self._set_user_environment(username)

    def _set_user_environment(self, username):
        for name in ('LOGNAME', 'USER', 'LNAME', 'USERNAME'):
            if name in os.environ:
                os.environ[name] = username

    def _ps(self, proc=constants.BROWSER):
        pscmd = 'ps -C %s -o pid --no-header | head -1' % proc
        return utils.system_output(pscmd)

    def run_once(self):
        self._set_user('chronos')

        bus = dbus.SystemBus()
        proxy = bus.get_object('org.chromium.SessionManager',
                               '/org/chromium/SessionManager')
        session_manager = dbus.Interface(proxy,
                                         'org.chromium.SessionManagerInterface')

        chrome_pid = self._ps()

        # Try DBus call and make sure it fails.
        try:
            # DBus cannot infer the type of an empty Python list.
            # Pass an empty dbus.Array with the correct signature, taken from
            # platform/login_manager/org.chromium.SessionManagerInterface.xml.
            empty_string_array = dbus.Array(signature="as")
            session_manager.EnableChromeTesting(True, empty_string_array)
        except dbus.exceptions.DBusException as dbe:
            logging.error(dbe)
        else:
            raise error.TestFail('DBus EnableChromeTesting call '
                                 'succeeded when it should fail.')

        # Make sure Chrome didn't restart.
        if chrome_pid != self._ps():
            raise error.TestFail('Chrome restarted during test.')

        self._reset_user()
