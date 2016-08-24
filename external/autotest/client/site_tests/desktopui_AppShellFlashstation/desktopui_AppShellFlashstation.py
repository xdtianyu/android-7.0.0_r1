# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error


class desktopui_AppShellFlashstation(test.test):
    """
    Checks that app_shell is up and that the flashstation app logged a message.
    """

    version = 1

    # Name of the app_shell binary.
    _APP_SHELL_EXECUTABLE = 'app_shell'

    # Regular expression matching a message logged by the app via console.log()
    # after it starts.
    _STARTUP_MESSAGE = 'Flashstation UI is ready'

    # File where the current app_shell instance logs the app's output.
    _LOG_FILE = '/var/log/ui/ui.LATEST'

    def run_once(self):
        """Runs the test."""

        if not utils.get_oldest_pid_by_name(self._APP_SHELL_EXECUTABLE):
            raise error.TestFail('Didn\'t find a process named "%s"' %
                                 self._APP_SHELL_EXECUTABLE)

        if not utils.grep(self._STARTUP_MESSAGE, self._LOG_FILE):
            raise error.TestFail('"%s" isn\'t present in %s' %
                                 (self._STARTUP_MESSAGE, self._LOG_FILE))
