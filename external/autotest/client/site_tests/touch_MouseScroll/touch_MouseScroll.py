# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import logging

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import chrome
from autotest_lib.client.cros import touch_playback_test_base


class touch_MouseScroll(touch_playback_test_base.touch_playback_test_base):
    """Plays back mouse scrolls and checks for correct page movement."""
    version = 1

    _MOUSE_DESCRIPTION = 'amazon_mouse.prop'
    _APPLE_MOUSE_DES = 'apple_mouse.prop'
    _EXPECTED_VALUE_1 = 16 # Expected value of one scroll wheel turn.
    _EXPECTED_DIRECTION = {'down': 1, 'up': -1, 'right': 1, 'left': -1}
    _TOLLERANCE = 4 # Fast scroll should go at least X times slow scroll.


    def _get_scroll_delta(self, name, expected_direction, scroll_vertical=True):
        """Playback the given test and return the amount the page moved.

        @param name: name of test filename.
        @param expected_direction: an integer that is + for down and - for up.
        @param scroll_vertical: True for vertical scroll,
                                False for horizontal scroll.

        @raise: TestFail if scrolling did not occur in expected direction.

        """
        self._set_default_scroll_position(scroll_vertical)
        self._playback(self._gest_file_path[name], touch_type='mouse')
        self._wait_for_scroll_position_to_settle(scroll_vertical)
        delta = self._get_scroll_position(scroll_vertical) - self._DEFAULT_SCROLL
        logging.info('Test %s: saw scroll delta of %d.  Expected direction %d.',
                     name, delta, expected_direction)

        if delta * expected_direction < 0:
            raise error.TestFail('Scroll was in wrong direction!  Delta '
                                 'for %s was %d.' % (name, delta))
        return delta


    def _verify_single_tick(self, direction, scroll_vertical=True):
        """Verify that using the scroll wheel goes the right distance.

        Expects a file (playback gesture file) named direction + '_1'.

        @param direction: string indicating direction up, down, right and left.
        @param scroll_vertical: scroll_vertical is True for vertical scroll
                                else False

        """
        name = direction + '_1'
        expected_direction = self._EXPECTED_DIRECTION[direction]
        expected_value = self._EXPECTED_VALUE_1 * expected_direction
        delta = self._get_scroll_delta(name, expected_direction,
                                       scroll_vertical)

        if delta != expected_value:
            raise error.TestFail('One tick scroll was wrong size: actual=%d, '
                                 'expected=%d.' % (delta, expected_value))


    def _verify_fast_vs_slow(self, direction, scroll_vertical=True):
        """Verify that fast scrolling goes farther than slow scrolling.

        Expects files (playback gesture file) named direction + '_slow'
        and direction + '_fast'.

        @param direction: string indicating direction up, down, right and left.
        @param scroll_vertical: True for vertical scroll,
                                False for horizontal scroll.

        """
        slow = direction + '_slow'
        fast = direction + '_fast'
        expected = self._EXPECTED_DIRECTION[direction]
        slow_delta = self._get_scroll_delta(slow, expected, scroll_vertical)
        fast_delta = self._get_scroll_delta(fast, expected, scroll_vertical)

        if abs(fast_delta) < self._TOLLERANCE * abs(slow_delta):
            raise error.TestFail('Fast scroll should be much farther than '
                                 'slow! (%s).  %d vs. %d.' %
                                  (direction, slow_delta, fast_delta))


    def run_once(self):
        """Entry point of this test."""

        # Link path for files to playback on DUT.
        self._gest_file_path = {}
        gestures_dir = os.path.join(self.bindir, 'gestures')
        for filename in os.listdir(gestures_dir):
            self._gest_file_path[filename] = os.path.join(gestures_dir,
                                                          filename)

        with chrome.Chrome() as cr:
            # Open test page.
            self._open_test_page(cr)

            # Emulate mouse with vertical scroll feature.
            mouse_file = os.path.join(self.bindir, self._MOUSE_DESCRIPTION)
            self._emulate_mouse(property_file=mouse_file)
            if not self._has_mouse:
                raise error.TestError('No USB mouse found on this device.')
            # In test page, position cursor to center.
            self._blocking_playback(self._gest_file_path['center_cursor'],
                                    touch_type='mouse')

            # Test vertical scrolling.
            for direction in ['down', 'up']:
                self._verify_single_tick(direction)
                self._verify_fast_vs_slow(direction)

            # Emulate mouse with horizontal scroll feature.
            apple_mouse_file = os.path.join(self.bindir, self._APPLE_MOUSE_DES)
            self._emulate_mouse(property_file=apple_mouse_file)
            if not self._has_mouse:
                raise error.TestError('No USB mouse found on this device.')

            # Test horizontal scrolling.
            for direction in ['right', 'left']:
                self._verify_single_tick(direction, scroll_vertical=False)
                self._verify_fast_vs_slow(direction, scroll_vertical=False)
