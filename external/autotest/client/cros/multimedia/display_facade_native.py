# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Facade to access the display-related functionality."""

import multiprocessing
import numpy
import os
import re
import time
from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import retry
from autotest_lib.client.cros import constants, sys_power
from autotest_lib.client.cros.graphics import graphics_utils
from autotest_lib.client.cros.multimedia import facade_resource
from autotest_lib.client.cros.multimedia import image_generator
from telemetry.internal.browser import web_contents

class TimeoutException(Exception):
    """Timeout Exception class."""
    pass


_FLAKY_CALL_RETRY_TIMEOUT_SEC = 60
_FLAKY_DISPLAY_CALL_RETRY_DELAY_SEC = 2

_retry_display_call = retry.retry(
        (KeyError, error.CmdError),
        timeout_min=_FLAKY_CALL_RETRY_TIMEOUT_SEC / 60.0,
        delay_sec=_FLAKY_DISPLAY_CALL_RETRY_DELAY_SEC)


class DisplayFacadeNative(object):
    """Facade to access the display-related functionality.

    The methods inside this class only accept Python native types.
    """

    CALIBRATION_IMAGE_PATH = '/tmp/calibration.svg'
    MINIMUM_REFRESH_RATE_EXPECTED = 25.0
    DELAY_TIME = 3

    def __init__(self, resource):
        """Initializes a DisplayFacadeNative.

        @param resource: A FacadeResource object.
        """
        self._resource = resource
        self._image_generator = image_generator.ImageGenerator()


    @facade_resource.retry_chrome_call
    def get_display_info(self):
        """Gets the display info from Chrome.system.display API.

        @return array of dict for display info.
        """
        extension = self._resource.get_extension(
                constants.MULTIMEDIA_TEST_EXTENSION)
        extension.ExecuteJavaScript('window.__display_info = null;')
        extension.ExecuteJavaScript(
                "chrome.system.display.getInfo(function(info) {"
                "window.__display_info = info;})")
        utils.wait_for_value(lambda: (
                extension.EvaluateJavaScript("window.__display_info") != None),
                expected_value=True)
        return extension.EvaluateJavaScript("window.__display_info")


    @facade_resource.retry_chrome_call
    def get_window_info(self):
        """Gets the current window info from Chrome.system.window API.

        @return a dict for the information of the current window.
        """
        extension = self._resource.get_extension()
        extension.ExecuteJavaScript('window.__window_info = null;')
        extension.ExecuteJavaScript(
                "chrome.windows.getCurrent(function(info) {"
                "window.__window_info = info;})")
        utils.wait_for_value(lambda: (
                extension.EvaluateJavaScript("window.__window_info") != None),
                expected_value=True)
        return extension.EvaluateJavaScript("window.__window_info")


    def _wait_for_display_options_to_appear(self, tab, display_index,
                                            timeout=16):
        """Waits for option.DisplayOptions to appear.

        The function waits until options.DisplayOptions appears or is timed out
                after the specified time.

        @param tab: the tab where the display options dialog is shown.
        @param display_index: index of the display.
        @param timeout: time wait for display options appear.

        @raise RuntimeError when display_index is out of range
        @raise TimeoutException when the operation is timed out.
        """

        tab.WaitForJavaScriptExpression(
                    "typeof options !== 'undefined' &&"
                    "typeof options.DisplayOptions !== 'undefined' &&"
                    "typeof options.DisplayOptions.instance_ !== 'undefined' &&"
                    "typeof options.DisplayOptions.instance_"
                    "       .displays_ !== 'undefined'", timeout)

        if not tab.EvaluateJavaScript(
                    "options.DisplayOptions.instance_.displays_.length > %d"
                    % (display_index)):
            raise RuntimeError('Display index out of range: '
                    + str(tab.EvaluateJavaScript(
                    "options.DisplayOptions.instance_.displays_.length")))

        tab.WaitForJavaScriptExpression(
                "typeof options.DisplayOptions.instance_"
                "         .displays_[%(index)d] !== 'undefined' &&"
                "typeof options.DisplayOptions.instance_"
                "         .displays_[%(index)d].id !== 'undefined' &&"
                "typeof options.DisplayOptions.instance_"
                "         .displays_[%(index)d].resolutions !== 'undefined'"
                % {'index': display_index}, timeout)


    def get_display_modes(self, display_index):
        """Gets all the display modes for the specified display.

        The modes are obtained from chrome://settings-frame/display via
        telemetry.

        @param display_index: index of the display to get modes from.

        @return: A list of DisplayMode dicts.

        @raise TimeoutException when the operation is timed out.
        """
        try:
            tab_descriptor = self.load_url('chrome://settings-frame/display')
            tab = self._resource.get_tab_by_descriptor(tab_descriptor)
            self._wait_for_display_options_to_appear(tab, display_index)
            return tab.EvaluateJavaScript(
                    "options.DisplayOptions.instance_"
                    "         .displays_[%(index)d].resolutions"
                    % {'index': display_index})
        finally:
            self.close_tab(tab_descriptor)


    def get_available_resolutions(self, display_index):
        """Gets the resolutions from the specified display.

        @return a list of (width, height) tuples.
        """
        # Start from M38 (refer to http://codereview.chromium.org/417113012),
        # a DisplayMode dict contains 'originalWidth'/'originalHeight'
        # in addition to 'width'/'height'.
        # OriginalWidth/originalHeight is what is supported by the display
        # while width/height is what is shown to users in the display setting.
        modes = self.get_display_modes(display_index)
        if modes:
            if 'originalWidth' in modes[0]:
                # M38 or newer
                # TODO(tingyuan): fix loading image for cases where original
                #                 width/height is different from width/height.
                return list(set([(mode['originalWidth'], mode['originalHeight'])
                        for mode in modes]))

        # pre-M38
        return [(mode['width'], mode['height']) for mode in modes
                if 'scale' not in mode]


    def get_first_external_display_index(self):
        """Gets the first external display index.

        @return the index of the first external display; False if not found.
        """
        # Get the first external and enabled display
        for index, display in enumerate(self.get_display_info()):
            if display['isEnabled'] and not display['isInternal']:
                return index
        return False


    def set_resolution(self, display_index, width, height, timeout=3):
        """Sets the resolution of the specified display.

        @param display_index: index of the display to set resolution for.
        @param width: width of the resolution
        @param height: height of the resolution
        @param timeout: maximal time in seconds waiting for the new resolution
                to settle in.
        @raise TimeoutException when the operation is timed out.
        """

        try:
            tab_descriptor = self.load_url('chrome://settings-frame/display')
            tab = self._resource.get_tab_by_descriptor(tab_descriptor)
            self._wait_for_display_options_to_appear(tab, display_index)

            tab.ExecuteJavaScript(
                    # Start from M38 (refer to CR:417113012), a DisplayMode dict
                    # contains 'originalWidth'/'originalHeight' in addition to
                    # 'width'/'height'. OriginalWidth/originalHeight is what is
                    # supported by the display while width/height is what is
                    # shown to users in the display setting.
                    """
                    var display = options.DisplayOptions.instance_
                              .displays_[%(index)d];
                    var modes = display.resolutions;
                    for (index in modes) {
                        var mode = modes[index];
                        if (mode.originalWidth == %(width)d &&
                                mode.originalHeight == %(height)d) {
                            chrome.send('setDisplayMode', [display.id, mode]);
                            break;
                        }
                    }
                    """
                    % {'index': display_index, 'width': width, 'height': height}
            )

            def _get_selected_resolution():
                modes = tab.EvaluateJavaScript(
                        """
                        options.DisplayOptions.instance_
                                 .displays_[%(index)d].resolutions
                        """
                        % {'index': display_index})
                for mode in modes:
                    if mode['selected']:
                        return (mode['originalWidth'], mode['originalHeight'])

            # TODO(tingyuan):
            # Support for multiple external monitors (i.e. for chromebox)
            end_time = time.time() + timeout
            while time.time() < end_time:
                r = _get_selected_resolution()
                if (width, height) == (r[0], r[1]):
                    return True
                time.sleep(0.1)
            raise TimeoutException('Failed to change resolution to %r (%r'
                                   ' detected)' % ((width, height), r))
        finally:
            self.close_tab(tab_descriptor)


    @_retry_display_call
    def get_external_resolution(self):
        """Gets the resolution of the external screen.

        @return The resolution tuple (width, height)
        """
        return graphics_utils.get_external_resolution()

    def get_internal_resolution(self):
        """Gets the resolution of the internal screen.

        @return The resolution tuple (width, height) or None if internal screen
                is not available
        """
        for display in self.get_display_info():
            if display['isInternal']:
                bounds = display['bounds']
                return (bounds['width'], bounds['height'])
        return None


    def set_content_protection(self, state):
        """Sets the content protection of the external screen.

        @param state: One of the states 'Undesired', 'Desired', or 'Enabled'
        """
        connector = self.get_external_connector_name()
        graphics_utils.set_content_protection(connector, state)


    def get_content_protection(self):
        """Gets the state of the content protection.

        @param output: The output name as a string.
        @return: A string of the state, like 'Undesired', 'Desired', or 'Enabled'.
                 False if not supported.
        """
        connector = self.get_external_connector_name()
        return graphics_utils.get_content_protection(connector)


    def get_external_crtc(self):
        """Gets the external crtc.

        @return The id of the external crtc."""
        return graphics_utils.get_external_crtc()


    def get_internal_crtc(self):
        """Gets the internal crtc.

        @retrun The id of the internal crtc."""
        return graphics_utils.get_internal_crtc()


    def get_output_rect(self, output):
        """Gets the size and position of the given output on the screen buffer.

        @param output: The output name as a string.

        @return A tuple of the rectangle (width, height, fb_offset_x,
                fb_offset_y) of ints.
        """
        regexp = re.compile(
                r'^([-A-Za-z0-9]+)\s+connected\s+(\d+)x(\d+)\+(\d+)\+(\d+)',
                re.M)
        match = regexp.findall(graphics_utils.call_xrandr())
        for m in match:
            if m[0] == output:
                return (int(m[1]), int(m[2]), int(m[3]), int(m[4]))
        return (0, 0, 0, 0)


    def take_internal_screenshot(self, path):
        """Takes internal screenshot.

        @param path: path to image file.
        """
        if utils.is_freon():
            self.take_screenshot_crtc(path, self.get_internal_crtc())
        else:
            output = self.get_internal_connector_name()
            box = self.get_output_rect(output)
            graphics_utils.take_screenshot_crop_x(path, box)
            return output, box  # for logging/debugging


    def take_external_screenshot(self, path):
        """Takes external screenshot.

        @param path: path to image file.
        """
        if utils.is_freon():
            self.take_screenshot_crtc(path, self.get_external_crtc())
        else:
            output = self.get_external_connector_name()
            box = self.get_output_rect(output)
            graphics_utils.take_screenshot_crop_x(path, box)
            return output, box  # for logging/debugging


    def take_screenshot_crtc(self, path, id):
        """Captures the DUT screenshot, use id for selecting screen.

        @param path: path to image file.
        @param id: The id of the crtc to screenshot.
        """

        graphics_utils.take_screenshot_crop(path, crtc_id=id)
        return True


    def take_tab_screenshot(self, output_path, url_pattern=None):
        """Takes a screenshot of the tab specified by the given url pattern.

        @param output_path: A path of the output file.
        @param url_pattern: A string of url pattern used to search for tabs.
                            Default is to look for .svg image.
        """
        if url_pattern is None:
            # If no URL pattern is provided, defaults to capture the first
            # tab that shows SVG image.
            url_pattern = '.svg'

        tabs = self._resource.get_tabs()
        for i in xrange(0, len(tabs)):
            if url_pattern in tabs[i].url:
                data = tabs[i].Screenshot(timeout=5)
                # Flip the colors from BGR to RGB.
                data = numpy.fliplr(data.reshape(-1, 3)).reshape(data.shape)
                data.tofile(output_path)
                break
        return True


    def toggle_mirrored(self):
        """Toggles mirrored."""
        graphics_utils.screen_toggle_mirrored()
        return True


    def hide_cursor(self):
        """Hides mouse cursor."""
        graphics_utils.hide_cursor()
        return True


    def is_mirrored_enabled(self):
        """Checks the mirrored state.

        @return True if mirrored mode is enabled.
        """
        return bool(self.get_display_info()[0]['mirroringSourceId'])


    def set_mirrored(self, is_mirrored):
        """Sets mirrored mode.

        @param is_mirrored: True or False to indicate mirrored state.
        @return True if success, False otherwise.
        """
        # TODO: Do some experiments to minimize waiting time after toggling.
        retries = 3
        while self.is_mirrored_enabled() != is_mirrored and retries > 0:
            self.toggle_mirrored()
            time.sleep(3)
            retries -= 1
        return self.is_mirrored_enabled() == is_mirrored


    def is_display_primary(self, internal=True):
        """Checks if internal screen is primary display.

        @param internal: is internal/external screen primary status requested
        @return boolean True if internal display is primary.
        """
        for info in self.get_display_info():
            if info['isInternal'] == internal and info['isPrimary']:
                return True
        return False


    def suspend_resume(self, suspend_time=10):
        """Suspends the DUT for a given time in second.

        @param suspend_time: Suspend time in second.
        """
        sys_power.do_suspend(suspend_time)
        return True


    def suspend_resume_bg(self, suspend_time=10):
        """Suspends the DUT for a given time in second in the background.

        @param suspend_time: Suspend time in second.
        """
        process = multiprocessing.Process(target=self.suspend_resume,
                                          args=(suspend_time,))
        process.start()
        return True


    @_retry_display_call
    def get_external_connector_name(self):
        """Gets the name of the external output connector.

        @return The external output connector name as a string, if any.
                Otherwise, return False.
        """
        return graphics_utils.get_external_connector_name()


    def get_internal_connector_name(self):
        """Gets the name of the internal output connector.

        @return The internal output connector name as a string, if any.
                Otherwise, return False.
        """
        return graphics_utils.get_internal_connector_name()


    def wait_external_display_connected(self, display):
        """Waits for the specified external display to be connected.

        @param display: The display name as a string, like 'HDMI1', or
                        False if no external display is expected.
        @return: True if display is connected; False otherwise.
        """
        result = utils.wait_for_value(self.get_external_connector_name,
                                      expected_value=display)
        return result == display


    @facade_resource.retry_chrome_call
    def move_to_display(self, display_index):
        """Moves the current window to the indicated display.

        @param display_index: The index of the indicated display.
        @return True if success.

        @raise TimeoutException if it fails.
        """
        display_info = self.get_display_info()
        if (display_index is False or
            display_index not in xrange(0, len(display_info)) or
            not display_info[display_index]['isEnabled']):
            raise RuntimeError('Cannot find the indicated display')
        target_bounds = display_info[display_index]['bounds']

        extension = self._resource.get_extension()
        # If the area of bounds is empty (here we achieve this by setting
        # width and height to zero), the window_sizer will automatically
        # determine an area which is visible and fits on the screen.
        # For more details, see chrome/browser/ui/window_sizer.cc
        # Without setting state to 'normal', if the current state is
        # 'minimized', 'maximized' or 'fullscreen', the setting of
        # 'left', 'top', 'width' and 'height' will be ignored.
        # For more details, see chrome/browser/extensions/api/tabs/tabs_api.cc
        extension.ExecuteJavaScript(
                """
                var __status = 'Running';
                chrome.windows.update(
                        chrome.windows.WINDOW_ID_CURRENT,
                        {left: %d, top: %d, width: 0, height: 0,
                         state: 'normal'},
                        function(info) {
                            if (info.left == %d && info.top == %d &&
                                info.state == 'normal')
                                __status = 'Done'; });
                """
                % (target_bounds['left'], target_bounds['top'],
                   target_bounds['left'], target_bounds['top'])
        )
        extension.WaitForJavaScriptExpression(
                "__status == 'Done'",
                web_contents.DEFAULT_WEB_CONTENTS_TIMEOUT)
        return True


    def is_fullscreen_enabled(self):
        """Checks the fullscreen state.

        @return True if fullscreen mode is enabled.
        """
        return self.get_window_info()['state'] == 'fullscreen'


    def set_fullscreen(self, is_fullscreen):
        """Sets the current window to full screen.

        @param is_fullscreen: True or False to indicate fullscreen state.
        @return True if success, False otherwise.
        """
        extension = self._resource.get_extension()
        if not extension:
            raise RuntimeError('Autotest extension not found')

        if is_fullscreen:
            window_state = "fullscreen"
        else:
            window_state = "normal"
        extension.ExecuteJavaScript(
                """
                var __status = 'Running';
                chrome.windows.update(
                        chrome.windows.WINDOW_ID_CURRENT,
                        {state: '%s'},
                        function() { __status = 'Done'; });
                """
                % window_state)
        utils.wait_for_value(lambda: (
                extension.EvaluateJavaScript('__status') == 'Done'),
                expected_value=True)
        return self.is_fullscreen_enabled() == is_fullscreen


    def load_url(self, url):
        """Loads the given url in a new tab. The new tab will be active.

        @param url: The url to load as a string.
        @return a str, the tab descriptor of the opened tab.
        """
        return self._resource.load_url(url)


    def load_calibration_image(self, resolution):
        """Opens a new tab and loads a full screen calibration
           image from the HTTP server.

        @param resolution: A tuple (width, height) of resolution.
        @return a str, the tab descriptor of the opened tab.
        """
        path = self.CALIBRATION_IMAGE_PATH
        self._image_generator.generate_image(resolution[0], resolution[1], path)
        os.chmod(path, 0644)
        tab_descriptor = self.load_url('file://%s' % path)
        return tab_descriptor


    def load_color_sequence(self, tab_descriptor, color_sequence):
        """Displays a series of colors on full screen on the tab.
        tab_descriptor is returned by any open tab API of display facade.
        e.g.,
        tab_descriptor = load_url('about:blank')
        load_color_sequence(tab_descriptor, color)

        @param tab_descriptor: Indicate which tab to test.
        @param color_sequence: An integer list for switching colors.
        @return A list of the timestamp for each switch.
        """
        tab = self._resource.get_tab_by_descriptor(tab_descriptor)
        color_sequence_for_java_script = (
                'var color_sequence = [' +
                ','.join("'#%06X'" % x for x in color_sequence) +
                '];')
        # Paints are synchronized to the fresh rate of the screen by
        # window.requestAnimationFrame.
        tab.ExecuteJavaScript(color_sequence_for_java_script + """
            function render(timestamp) {
                window.timestamp_list.push(timestamp);
                if (window.count < color_sequence.length) {
                    document.body.style.backgroundColor =
                            color_sequence[count];
                    window.count++;
                    window.requestAnimationFrame(render);
                }
            }
            window.count = 0;
            window.timestamp_list = [];
            window.requestAnimationFrame(render);
            """)

        # Waiting time is decided by following concerns:
        # 1. MINIMUM_REFRESH_RATE_EXPECTED: the minimum refresh rate
        #    we expect it to be. Real refresh rate is related to
        #    not only hardware devices but also drivers and browsers.
        #    Most graphics devices support at least 60fps for a single
        #    monitor, and under mirror mode, since the both frames
        #    buffers need to be updated for an input frame, the refresh
        #    rate will decrease by half, so here we set it to be a
        #    little less than 30 (= 60/2) to make it more tolerant.
        # 2. DELAY_TIME: extra wait time for timeout.
        tab.WaitForJavaScriptExpression(
                'window.count == color_sequence.length',
                (len(color_sequence) / self.MINIMUM_REFRESH_RATE_EXPECTED)
                + self.DELAY_TIME)
        return tab.EvaluateJavaScript("window.timestamp_list")


    def close_tab(self, tab_descriptor):
        """Disables fullscreen and closes the tab of the given tab descriptor.
        tab_descriptor is returned by any open tab API of display facade.
        e.g.,
        1.
        tab_descriptor = load_url(url)
        close_tab(tab_descriptor)

        2.
        tab_descriptor = load_calibration_image(resolution)
        close_tab(tab_descriptor)

        @param tab_descriptor: Indicate which tab to be closed.
        """
        # set_fullscreen(False) is necessary here because currently there
        # is a bug in tabs.Close(). If the current state is fullscreen and
        # we call close_tab() without setting state back to normal, it will
        # cancel fullscreen mode without changing system configuration, and
        # so that the next time someone calls set_fullscreen(True), the
        # function will find that current state is already 'fullscreen'
        # (though it is not) and do nothing, which will break all the
        # following tests.
        self.set_fullscreen(False)
        self._resource.close_tab(tab_descriptor)
        return True
