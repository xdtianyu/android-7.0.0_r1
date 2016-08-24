# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import time

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import chrome
from autotest_lib.client.cros import touch_playback_test_base


class touch_TouchscreenTaps(touch_playback_test_base.touch_playback_test_base):
    """Checks that touchscreen presses are translated into clicks."""
    version = 1

    _TEST_TIMEOUT = 1  # Number of seconds the test will wait for a click.
    _CLICK_NAME = 'tap'


    def _check_for_click(self):
        """Playback and check whether click occurred.  Fail if not.

        @raises: TestFail if no click occurred.

        """
        self._reload_page()
        self._blocking_playback(filepath=self._filepaths[self._CLICK_NAME],
                                touch_type='touchscreen')
        time.sleep(self._TEST_TIMEOUT)
        actual_count = int(self._tab.EvaluateJavaScript('clickCount'))
        if actual_count is not 1:
            raise error.TestFail('Saw %d clicks!' % actual_count)


    def _is_testable(self):
        """Return True if test can run on this device, else False.

        @raises: TestError if host has no touchscreen.

        """
        # Raise error if no touchscreen detected.
        if not self._has_touchscreen:
            raise error.TestError('No touchscreen found on this device!')

        # Check if playback files are available on DUT to run test.
        self._filepaths = self._find_test_files(
                'touchscreen', [self._CLICK_NAME])
        if not self._filepaths:
            logging.info('Missing gesture files, Aborting test.')
            return False

        return True


    def run_once(self):
        """Entry point of this test."""
        if not self._is_testable():
            return

        # Log in and start test.
        with chrome.Chrome() as cr:
            self._open_test_page(cr)
            self._check_for_click()
