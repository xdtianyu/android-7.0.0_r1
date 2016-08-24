# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, os, signal, time

import common
from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import constants, cros_logging, cros_ui, login

class desktopui_HangDetector(test.test):
    """
    This class enables browser process hang detection, simulates a hang
    by sending a SIGSTOP to the browser, and then checks to see that it
    got killed and restarted successfully -- without the UI getting bounced.
    """
    version = 1


    def initialize(self):
        self._pauser = cros_logging.LogRotationPauser()
        self._pauser.begin()


    def _get_oldest_pid_by_name(self, name):
        try:
            pid = utils.get_oldest_pid_by_name(name)
            logging.debug('Found %d for %s', pid, name)
        except error.CmdError as e:
            raise error.TestError('Could not find pid of %s: %r' % (name, e))
        except ValueError as e:
            raise error.TestError('Got bad pid looking up %s: %r' % (name, e))
        if not pid:
            raise error.TestError('Got no pid looking up %s' % name)
        return pid


    def run_once(self):
        # Create magic file to enable browser liveness checking and
        # bounce the session manager to pick up the flag file.
        cros_ui.stop()
        os.mknod(constants.ENABLE_BROWSER_HANG_DETECTION_FILE)
        cros_ui.start()

        browser_pid = self._get_oldest_pid_by_name(constants.BROWSER)
        sm_pid = self._get_oldest_pid_by_name(constants.SESSION_MANAGER)

        # Reading the log is the best way to watch for the hang detector.
        reader = cros_logging.LogReader()
        reader.set_start_by_current()

        # To simulate a hang, STOP the browser and wait for it to get
        # hit by the session manager.  It won't actually exit until it gets
        # a SIGCONT, though.
        try:
            os.kill(browser_pid, signal.SIGSTOP)  # Simulate hang.
        except OSError as e:
            raise error.TestError('Cannot STOP browser: %r' % e)

        # Watch for hang detection.
        utils.poll_for_condition(
            condition=lambda: reader.can_find('Aborting browser process.'),
            exception=utils.TimeoutError('Waiting for hang detector.'),
            sleep_interval=5,
            timeout=60)

        try:
            os.kill(browser_pid, signal.SIGCONT)  # Allow browser to die.
        except OSError as e:
            raise error.TestError('Cannot CONT browser: %r' % e)

        # Wait for old browser process to be gone.
        utils.poll_for_condition(
            condition= lambda: utils.pid_is_alive(browser_pid),
            exception=utils.TimeoutError(
                'Browser does not seem to have restarted!'),
            timeout=60)

        # Wait for new browser to come up.
        login.wait_for_browser()
        if sm_pid != self._get_oldest_pid_by_name(constants.SESSION_MANAGER):
            raise error.TestFail('session_manager seems to have restarted')


    def cleanup(self):
        if os.path.exists(constants.ENABLE_BROWSER_HANG_DETECTION_FILE):
            os.remove(constants.ENABLE_BROWSER_HANG_DETECTION_FILE)
        self._pauser.end()
