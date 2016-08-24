# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import gobject, logging
from dbus.exceptions import DBusException
from dbus.mainloop.glib import DBusGMainLoop

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import chrome, session_manager
from autotest_lib.client.cros import constants


class desktopui_ExitOnSupervisedUserCrash(test.test):
    """Sign in, indicate that a supervised user is being created, then crash."""
    version = 1

    _SESSION_STOP_TIMEOUT = 60


    def initialize(self):
        super(desktopui_ExitOnSupervisedUserCrash, self).initialize()


    def run_once(self):
        listener = session_manager.SessionSignalListener(gobject.MainLoop())
        with chrome.Chrome():
            sm = session_manager.connect(DBusGMainLoop(set_as_default=True))
            # Tell session_manager that we're going all the way through
            # creating a supervised user.
            sm.HandleSupervisedUserCreationStarting()
            sm.HandleSupervisedUserCreationFinished()
            # Crashing the browser should not end the session, as creating the
            # user is finished.
            utils.nuke_process_by_name(constants.BROWSER)

            # We should still be able to talk to the session_manager,
            # and it should indicate that we're still inside a user session.
            try:
                state = sm.RetrieveSessionState()
            except DBusException as e:
                raise error.TestError('Failed to retrieve session state: ', e)
            if state != 'started':
                raise error.TestFail('Session should not have ended: ', state)

            # Start listening to stop signal before the session gets killed.
            listener.listen_for_session_state_change('stopped')

            # Tell session_manager that a supervised user is being set up,
            # and kill it in the middle. Session should die.
            sm.HandleSupervisedUserCreationStarting()
            nuke_browser_error = None
            try:
                utils.nuke_process_by_name(constants.BROWSER)
            except error.AutoservPidAlreadyDeadError as e:
                nuke_browser_error = e
                logging.warning('Browser may have crashed untimely: ', e)

            try:
                listener.wait_for_signals(desc='Session stopped.',
                                          timeout=self._SESSION_STOP_TIMEOUT)
            except utils.TimeoutError as actual_problem:
                if nuke_browser_error is not None:
                    actual_problem = nuke_browser_error
                raise error.TestFail(actual_problem)
