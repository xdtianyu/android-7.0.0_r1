# Copyright (c) 2009 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, os, time
from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import cros_ui

class desktopui_KillRestart(test.test):
    """Validate that the given binary can crash and get restarted."""
    version = 1

    def initialize(self):
        """Clear out respawn timestamp files."""
        cros_ui.clear_respawn_state()

    def run_once(self, binary = 'chrome'):
        # Ensure the binary is running.
        utils.poll_for_condition(
            lambda: os.system('pgrep %s >/dev/null' % binary) == 0,
            error.TestFail('%s is not running at start of test' % binary),
            timeout=60)

        # Try to kill all running instances of the binary.
        try:
            utils.system('pkill -KILL %s' % binary)
        except error.CmdError, e:
            logging.debug(e)
            raise error.TestFail('%s is not running before kill' % binary)

        # Check if the binary is running again (using os.system(), since it
        # doesn't raise an exception if the command fails).
        utils.poll_for_condition(
            lambda: os.system('pgrep %s >/dev/null' % binary) == 0,
            error.TestFail('%s is not running after kill' % binary),
            timeout=60)


    def cleanup(self):
        """Ensure that state from testing is cleared out."""
        cros_ui.clear_respawn_state()
