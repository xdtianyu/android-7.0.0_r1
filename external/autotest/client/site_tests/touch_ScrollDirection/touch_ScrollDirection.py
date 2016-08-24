# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import chrome
from autotest_lib.client.cros import touch_playback_test_base


class touch_ScrollDirection(touch_playback_test_base.touch_playback_test_base):
    """Plays back scrolls and checks for correct page movement."""
    version = 1

    _DIRECTIONS = ['down', 'up', 'right', 'left']
    _REVERSES = {'down': 'up', 'up': 'down', 'right': 'left', 'left': 'right'}
    _FILENAME_FMT_STR = 'scroll-%s'


    def _check_scroll_direction(self, filepath, expected):
        """Playback and raise error if scrolling does not match down value.

        @param filepath: Gesture file's complete path for playback.
        @param expected: String, expected direction in which test page scroll
                         should move for the gesture file being played.

        @raises TestFail if actual scrolling did not match expected.

        """
        is_vertical = expected == 'up' or expected == 'down'
        is_down_or_right = expected == 'down' or expected == 'right'

        self._set_default_scroll_position(is_vertical)
        self._playback(filepath)
        self._wait_for_scroll_position_to_settle(is_vertical)
        delta = self._get_scroll_position(is_vertical) - self._DEFAULT_SCROLL
        logging.info('Scroll delta was %d', delta)

        # Check if scrolling went in correct direction.
        if ((is_down_or_right and delta <= 0) or
            (not is_down_or_right and delta >= 0)):
            raise error.TestFail('Page scroll was in wrong direction! '
                                 'Delta=%d, Australian=%s, Touchscreen=%s'
                                  % (delta, self._australian_state,
                                     self._has_touchscreen))


    def _verify_scrolling(self):
        """Check scrolling direction for down then up."""

        if not self._australian_state:
            for direction in self._DIRECTIONS:
                self._check_scroll_direction(self._filepaths[direction],
                                             direction)
        else:
            for direction in self._DIRECTIONS:
                self._check_scroll_direction(self._filepaths[direction],
                                             self._REVERSES[direction])


    def _is_testable(self):
        """Return True if test can run on this device, else False.

        @raises: TestError if host has no touchpad when it should.

        """
        # Raise error if no touchpad detected.
        if not self._has_touchpad:
            raise error.TestError('No touchpad found on this device!')

        # Check if playback files are available on DUT to run test.
        self._filepaths = self._find_test_files_from_directions(
                'touchpad', self._FILENAME_FMT_STR, self._DIRECTIONS)
        if not self._filepaths:
            logging.info('Missing gesture files, Aborting test.')
            return False

        return True


    def run_once(self):
        """Entry point of this test."""
        if not self._is_testable():
            return

        # Log in and start test.
        with chrome.Chrome(autotest_ext=True) as cr:
            # Setup.
            self._set_autotest_ext(cr.autotest_ext)
            self._open_test_page(cr)
            self._emulate_mouse()
            self._center_cursor()

            # Check default scroll - Australian for touchscreens.
            self._australian_state = self._has_touchscreen
            logging.info('Expecting Australian=%s', self._australian_state)
            self._verify_scrolling()

            # Toggle Australian scrolling and check again.
            self._australian_state = not self._australian_state
            self._set_australian_scrolling(value=self._australian_state)
            self._verify_scrolling()
