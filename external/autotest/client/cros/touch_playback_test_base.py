# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os
import time

from autotest_lib.client.bin import test
from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.input_playback import input_playback


class touch_playback_test_base(test.test):
    """Base class for touch tests involving playback."""
    version = 1

    _INPUTCONTROL = '/opt/google/input/inputcontrol'
    _DEFAULT_SCROLL = 5000


    @property
    def _has_touchpad(self):
        """True if device under test has a touchpad; else False."""
        return self.player.has('touchpad')


    @property
    def _has_touchscreen(self):
        """True if device under test has a touchscreen; else False."""
        return self.player.has('touchscreen')


    @property
    def _has_mouse(self):
        """True if device under test has or emulates a USB mouse; else False."""
        return self.player.has('mouse')


    def warmup(self, mouse_props=None):
        """Test setup.

        Instantiate player object to find touch devices, if any.
        These devices can be used for playback later.
        Emulate a USB mouse if a property file is provided.
        Check if the inputcontrol script is avaiable on the disk.

        @param mouse_props: optional property file for a mouse to emulate.
                            Created using 'evemu-describe /dev/input/X'.

        """
        self.player = input_playback.InputPlayback()
        if mouse_props:
            self.player.emulate(input_type='mouse', property_file=mouse_props)
        self.player.find_connected_inputs()

        self._autotest_ext = None
        self._has_inputcontrol = os.path.isfile(self._INPUTCONTROL)
        self._platform = utils.get_board()


    def _find_test_files(self, input_type, gestures):
        """Determine where the test files are.

        Expected file format is: <boardname>_<input type>_<hwid>_<gesture name>
            e.g. samus_touchpad_164.17_scroll_down

        @param input_type: device type, e.g. 'touchpad'
        @param gestures: list of gesture name strings used in filename

        @returns: None if not all files are found.  Dictionary of filepaths if
                  they are found, indexed by gesture names as given.
        @raises: error.TestError if no hw_id is found.

        """
        hw_id = self.player.devices[input_type].hw_id
        if not hw_id:
            raise error.TestError('No valid hw_id for this %s!' % input_type)

        filepaths = {}
        gesture_dir = os.path.join(self.bindir, 'gestures')
        for gesture in gestures:
            filename = '%s_%s_%s_%s' % (self._platform, input_type, hw_id,
                                        gesture)
            filepath = os.path.join(gesture_dir, filename)
            if not os.path.exists(filepath):
                logging.info('Did not find %s!', filepath)
                return None
            filepaths[gesture] = filepath

        return filepaths


    def _find_test_files_from_directions(self, input_type, fmt_str, directions):
        """Find test files given a list of directions and gesture name format.

        @param input_type: device type, e.g. 'touchpad'
        @param fmt_str: format string for filename, e.g. 'scroll-%s'
        @param directions: list of directions for fmt_string

        @returns: None if not all files are found.  Dictionary of filepaths if
                  they are found, indexed by directions as given.
        @raises: error.TestError if no hw_id is found.

        """
        gestures = [fmt_str % d for d in directions]
        temp_filepaths = self._find_test_files(input_type, gestures)

        filepaths = {}
        if temp_filepaths:
            filepaths = {d: temp_filepaths[fmt_str % d] for d in directions}

        return filepaths


    def _emulate_mouse(self, property_file=None):
        """Emulate a mouse with the given property file.

        player will use default mouse if no file is provided.

        """
        self.player.emulate(input_type='mouse', property_file=property_file)
        self.player.find_connected_inputs()
        if not self._has_mouse:
            raise error.TestError('Mouse emulation failed!')


    def _playback(self, filepath, touch_type='touchpad'):
        """Playback a given input file on the given input."""
        self.player.playback(filepath, touch_type)


    def _blocking_playback(self, filepath, touch_type='touchpad'):
        """Playback a given input file on the given input; block until done."""
        self.player.blocking_playback(filepath, touch_type)


    def _set_touch_setting_by_inputcontrol(self, setting, value):
        """Set a given touch setting the given value by inputcontrol.

        @param setting: Name of touch setting, e.g. 'tapclick'.
        @param value: True for enabled, False for disabled.

        """
        cmd_value = 1 if value else 0
        utils.run('%s --%s %d' % (self._INPUTCONTROL, setting, cmd_value))
        logging.info('%s turned %s.', setting, 'on' if value else 'off')


    def _set_touch_setting(self, inputcontrol_setting, autotest_ext_setting,
                           value):
        """Set a given touch setting the given value.

        @param inputcontrol_setting: Name of touch setting for the inputcontrol
                                     script, e.g. 'tapclick'.
        @param autotest_ext_setting: Name of touch setting for the autotest
                                     extension, e.g. 'TapToClick'.
        @param value: True for enabled, False for disabled.

        """
        if self._has_inputcontrol:
            self._set_touch_setting_by_inputcontrol(inputcontrol_setting, value)
        elif self._autotest_ext is not None:
            self._autotest_ext.EvaluateJavaScript(
                    'chrome.autotestPrivate.set%s(%s);'
                    % (autotest_ext_setting, ("%s" % value).lower()))
            # TODO: remove this sleep once checking for value is available.
            time.sleep(1)
        else:
            raise error.TestFail('Both inputcontrol and the autotest '
                                 'extension are not availble.')


    def _set_australian_scrolling(self, value):
        """Set australian scrolling to the given value.

        @param value: True for enabled, False for disabled.

        """
        self._set_touch_setting('australian_scrolling', 'NaturalScroll', value)


    def _set_tap_to_click(self, value):
        """Set tap-to-click to the given value.

        @param value: True for enabled, False for disabled.

        """
        self._set_touch_setting('tapclick', 'TapToClick', value)


    def _set_tap_dragging(self, value):
        """Set tap dragging to the given value.

        @param value: True for enabled, False for disabled.

        """
        self._set_touch_setting('tapdrag', 'TapDragging', value)


    def _reload_page(self):
        """Reloads test page.  Presuposes self._tab.

        @raise: TestError if page is not reset.

        """
        self._tab.Navigate(self._tab.url)
        self._wait_for_page_ready()


    def _set_autotest_ext(self, ext):
        """Set the autotest extension.

        @ext: the autotest extension object.

        """
        self._autotest_ext = ext


    def _open_test_page(self, cr, filename='test_page.html'):
        """Prepare test page for testing.  Set self._tab with page.

        @param cr: chrome.Chrome() object
        @param filename: name of file in self.bindir to open

        """
        cr.browser.platform.SetHTTPServerDirectories(self.bindir)
        self._tab = cr.browser.tabs[0]
        self._tab.Navigate(cr.browser.platform.http_server.UrlOf(
                os.path.join(self.bindir, filename)))
        self._wait_for_page_ready()


    def _wait_for_page_ready(self):
        """Wait for a variable pageReady on the test page to be true.

        Presuposes self._tab and a pageReady variable.

        @raises error.TestError if page is not ready after timeout.

        """
        self._tab.WaitForDocumentReadyStateToBeComplete()
        utils.poll_for_condition(
                lambda: self._tab.EvaluateJavaScript('pageReady'),
                exception=error.TestError('Test page is not ready!'))


    def _center_cursor(self):
        """Playback mouse movement to center cursor.

        Requres that self._emulate_mouse() has been called.

        """
        self.player.blocking_playback_of_default_file(
                'mouse_center_cursor_gesture', input_type='mouse')


    def _set_scroll(self, value, scroll_vertical=True):
        """Set scroll position to given value.  Presuposes self._tab.

        @param scroll_vertical: True for vertical scroll,
                                False for horizontal Scroll.
        @param value: True for enabled, False for disabled.

         """
        if scroll_vertical:
            self._tab.ExecuteJavaScript(
                'document.body.scrollTop=%s' % value)
        else:
            self._tab.ExecuteJavaScript(
                'document.body.scrollLeft=%s' % value)


    def _set_default_scroll_position(self, scroll_vertical=True):
        """Set scroll position of page to default.  Presuposes self._tab.

        @param scroll_vertical: True for vertical scroll,
                                False for horizontal Scroll.
        @raise: TestError if page is not set to default scroll position

        """
        total_tries = 2
        for i in xrange(total_tries):
            try:
                self._set_scroll(self._DEFAULT_SCROLL, scroll_vertical)
                self._wait_for_default_scroll_position(scroll_vertical)
            except error.TestError as e:
                if i == total_tries - 1:
                   pos = self._get_scroll_position(scroll_vertical)
                   logging.error('SCROLL POSITION: %s', pos)
                   raise e
            else:
                 break


    def _get_scroll_position(self, scroll_vertical=True):
        """Return current scroll position of page.  Presuposes self._tab.

        @param scroll_vertical: True for vertical scroll,
                                False for horizontal Scroll.

        """
        if scroll_vertical:
            return int(self._tab.EvaluateJavaScript('document.body.scrollTop'))
        else:
            return int(self._tab.EvaluateJavaScript('document.body.scrollLeft'))


    def _wait_for_default_scroll_position(self, scroll_vertical=True):
        """Wait for page to be at the default scroll position.

        @param scroll_vertical: True for vertical scroll,
                                False for horizontal scroll.

        @raise: TestError if page either does not move or does not stop moving.

        """
        utils.poll_for_condition(
                lambda: self._get_scroll_position(
                        scroll_vertical) == self._DEFAULT_SCROLL,
                exception=error.TestError('Page not set to default scroll!'))


    def _wait_for_scroll_position_to_settle(self, scroll_vertical=True):
        """Wait for page to move and then stop moving.

        @param scroll_vertical: True for Vertical scroll and
                                False for horizontal scroll.

        @raise: TestError if page either does not move or does not stop moving.

        """
        # Wait until page starts moving.
        utils.poll_for_condition(
                lambda: self._get_scroll_position(
                        scroll_vertical) != self._DEFAULT_SCROLL,
                exception=error.TestError('No scrolling occurred!'), timeout=30)

        # Wait until page has stopped moving.
        self._previous = self._DEFAULT_SCROLL
        def _movement_stopped():
            current = self._get_scroll_position()
            result = current == self._previous
            self._previous = current
            return result

        utils.poll_for_condition(
                lambda: _movement_stopped(), sleep_interval=1,
                exception=error.TestError('Page did not stop moving!'),
                timeout=30)


    def cleanup(self):
        self.player.close()
