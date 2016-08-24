# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import constants, cros_ui


class UIStopped(Exception):
    """Raised when the UI seems to have stopped respawning."""
    pass


class desktopui_CrashyReboot(test.test):
    """Drive device to handle a too-crashy UI.

    Run by desktopui_CrashyRebootServer.
    """
    version = 1

    UNREASONABLY_HIGH_RESPAWN_COUNT=90


    def _nuke_browser_with_prejudice_and_check_for_ui_stop(self):
        """Nuke the browser with prejudice, check to see if the UI is down."""
        try:
            utils.nuke_process_by_name(constants.BROWSER, with_prejudice=True)
        except error.AutoservPidAlreadyDeadError:
            pass
        return not cros_ui.is_up()


    def _nuke_browser_until_ui_goes_down(self):
        """Nuke the browser continuously until it stops respawning.

        @raises utils.TimeoutError if the ui doesn't stop respawning.
        """
        utils.poll_for_condition(
            condition=self._nuke_browser_with_prejudice_and_check_for_ui_stop,
            timeout=60,
            desc='ui to stop respawning, or the device to reboot')


    def run_once(self, expect_reboot=False):
        # Ensure the UI is running.
        logging.debug('Restarting UI to ensure that it\'s running.')
        cros_ui.stop(allow_fail=True)
        cros_ui.start(wait_for_login_prompt=True)

        # Since there is no 100% reliable way to determine that the
        # browser process we're interested in is gone, we need to use
        # a polling interval to continuously send KILL signals. This
        # puts the test code in an unavoidable race with the UI
        # respawning logic being tested. If the UI is down at the
        # instant we check, it could mean that the UI is done
        # respawning, the UI is about to respawn, or the device could
        # already be rebooting. In all likelihood, the UI is coming
        # back and we'll need to kill it all over again. This is why
        # the code below polls the UI status for a number of seconds:
        # to be more confident that the UI went down and is staying down.
        try:
            while True:
                utils.poll_for_condition(condition=cros_ui.is_up,
                                         timeout=5,
                                         exception=UIStopped('As expected'))
                self._nuke_browser_until_ui_goes_down()
        except UIStopped:
            pass
        except utils.TimeoutError as te:
            raise error.TestFail(te)

        if expect_reboot:
            raise error.TestFail('UI stopped respawning instead of rebooting.')


    def cleanup(self):
        # If the UI is already up, we want to tolerate that.
        cros_ui.start(allow_fail=True)
