# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import chrome
from autotest_lib.client.cros import touch_playback_test_base


class touch_TabSwitch(touch_playback_test_base.touch_playback_test_base):
    """Test to verify the three finger tab switching touchpad gesture."""
    version = 1

    # Devices with older touchpads that do not recognize 3+ fingers.
    _INVALID_BOARDS = ['x86-alex', 'x86-alex_he', 'x86-zgb', 'x86-zgb_he',
                       'x86-mario', 'stout']

    _DIRECTIONS = ['left', 'right']

    def _is_testable(self):
        """Returns True if the test can run on this device, else False."""
        if not self._has_touchpad:
            raise error.TestError('No touchpad found on this device!')

        if self._platform in self._INVALID_BOARDS:
            logging.info('Device does not support this gesture; aborting.')
            return False

        # Check if playback files are available on DUT to run test.
        self._filepaths = self._find_test_files_from_directions(
                'touchpad', 'three-finger-swipe-%s', self._DIRECTIONS)
        if not self._filepaths:
            logging.info('Missing gesture files, Aborting test.')
            return False

        return True

    def _set_up_tabs(self, cr):
        """Open two additional tabs for this test (total of three).

        @raises TestError if browser doesn't end up with correct tab count.

        """
        self._tabs = cr.browser.tabs
        tab_count = 3
        for i in xrange(1, tab_count):
            tab = cr.browser.tabs.New()

        if len(self._tabs) != tab_count:
            raise error.TestError('Expected %s tabs but found %s!' % (
                    tab_count, len(self._tabs)))

    def _require_active(self, index):
        """Fail the test if the index-th tab is not active.

        @param index: integer representing the position of our tab.

        @raises: TestFail if the tab is not active as expected.

        """
        tab = self._tabs[index]
        if tab.EvaluateJavaScript('document.hidden') == 'False':
            raise error.TestFail('Expected page %s to be active!' % index)

    def _check_tab_switch(self):
        """ Verify correct tab switching behavior.

        Regardless of australian scrolling setting, moving three fingers to
        the left will set the tab to the left as the active tab.
        Attempting to move past the last tab on either end will not wrap.

        Expects the third (and last) tab to be active.

        """
        for tab_index in [1, 0, 0]:
            self._blocking_playback(touch_type='touchpad',
                                    filepath=self._filepaths['left'])
            self._require_active(tab_index)

        for tab_index in [1, 2, 2]:
            self._blocking_playback(touch_type='touchpad',
                                    filepath=self._filepaths['right'])
            self._require_active(tab_index)

    def run_once(self):
        """Entry point of this test."""
        if not self._is_testable():
            return

        # Log in and start test.
        with chrome.Chrome(autotest_ext=True) as cr:
            self._set_autotest_ext(cr.autotest_ext)
            self._set_up_tabs(cr)

            self._check_tab_switch()

            # Toggle Australian scrolling and test again.
            new_australian_state = not self._has_touchscreen
            self._set_australian_scrolling(value=new_australian_state)
            self._check_tab_switch()
