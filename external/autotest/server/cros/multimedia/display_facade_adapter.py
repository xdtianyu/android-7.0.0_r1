# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""An adapter to remotely access the display facade on DUT."""

import logging
import os
import tempfile
import xmlrpclib

from PIL import Image

from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.multimedia.display_info import DisplayInfo


class DisplayFacadeRemoteAdapter(object):
    """DisplayFacadeRemoteAdapter is an adapter to remotely control DUT display.

    The Autotest host object representing the remote DUT, passed to this
    class on initialization, can be accessed from its _client property.

    """
    def __init__(self, host, remote_facade_proxy):
        """Construct a DisplayFacadeRemoteAdapter.

        @param host: Host object representing a remote host.
        @param remote_facade_proxy: RemoteFacadeProxy object.
        """
        self._client = host
        self._proxy = remote_facade_proxy


    @property
    def _display_proxy(self):
        """Gets the proxy to DUT display facade.

        @return XML RPC proxy to DUT display facade.
        """
        return self._proxy.display


    def get_external_connector_name(self):
        """Gets the name of the external output connector.

        @return The external output connector name as a string; False if nothing
                is connected.
        """
        return self._display_proxy.get_external_connector_name()


    def get_internal_connector_name(self):
        """Gets the name of the internal output connector.

        @return The internal output connector name as a string; False if nothing
                is connected.
        """
        return self._display_proxy.get_internal_connector_name()


    def move_to_display(self, display_index):
        """Moves the current window to the indicated display.

        @param display_index: The index of the indicated display.
        """
        self._display_proxy.move_to_display(display_index)


    def set_fullscreen(self, is_fullscreen):
        """Sets the current window to full screen.

        @param is_fullscreen: True or False to indicate fullscreen state.
        @return True if success, False otherwise.
        """
        return self._display_proxy.set_fullscreen(is_fullscreen)


    def load_url(self, url):
        """Loads the given url in a new tab. The new tab will be active.

        @param url: The url to load as a string.
        @return a str, the tab descriptor of the opened tab.
        """
        return self._display_proxy.load_url(url)


    def load_calibration_image(self, resolution):
        """Load a full screen calibration image from the HTTP server.

        @param resolution: A tuple (width, height) of resolution.
        @return a str, the tab descriptor of the opened tab.
        """
        return self._display_proxy.load_calibration_image(resolution)


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
        return self._display_proxy.load_color_sequence(tab_descriptor,
                                                       color_sequence)


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

        @param tab_descriptor: Indicate which tab to close.
        """
        self._display_proxy.close_tab(tab_descriptor)


    def is_mirrored_enabled(self):
        """Checks the mirrored state.

        @return True if mirrored mode is enabled.
        """
        return self._display_proxy.is_mirrored_enabled()


    def set_mirrored(self, is_mirrored):
        """Sets mirrored mode.

        @param is_mirrored: True or False to indicate mirrored state.
        @throws error.TestError when the call fails.
        """
        if not self._display_proxy.set_mirrored(is_mirrored):
            raise error.TestError('Failed to set_mirrored(%s)' % is_mirrored)


    def is_display_primary(self, internal=True):
        """Checks if internal screen is primary display.

        @param internal: is internal/external screen primary status requested
        @return boolean True if internal display is primary.
        """
        return self._display_proxy.is_display_primary(internal)


    def suspend_resume(self, suspend_time=10):
        """Suspends the DUT for a given time in second.

        @param suspend_time: Suspend time in second, default: 10s.
        """
        try:
            self._display_proxy.suspend_resume(suspend_time)
        except xmlrpclib.Fault as e:
            # Log suspend/resume errors but continue the test.
            logging.error('suspend_resume error: %s', str(e))


    def suspend_resume_bg(self, suspend_time=10):
        """Suspends the DUT for a given time in second in the background.

        @param suspend_time: Suspend time in second, default: 10s.
        """
        # TODO(waihong): Use other general API instead of this RPC.
        self._display_proxy.suspend_resume_bg(suspend_time)


    def wait_external_display_connected(self, display):
        """Waits for the specified display to be connected.

        @param display: The display name as a string, like 'HDMI1', or
                        False if no external display is expected.
        @return: True if display is connected; False otherwise.
        """
        return self._display_proxy.wait_external_display_connected(display)


    def hide_cursor(self):
        """Hides mouse cursor by sending a keystroke."""
        self._display_proxy.hide_cursor()


    def set_content_protection(self, state):
        """Sets the content protection of the external screen.

        @param state: One of the states 'Undesired', 'Desired', or 'Enabled'
        """
        self._display_proxy.set_content_protection(state)


    def get_content_protection(self):
        """Gets the state of the content protection.

        @param output: The output name as a string.
        @return: A string of the state, like 'Undesired', 'Desired', or 'Enabled'.
                 False if not supported.
        """
        return self._display_proxy.get_content_protection()


    def _take_screenshot(self, screenshot_func):
        """Gets screenshot from DUT.

        @param screenshot_func: function to take a screenshot and save the image
                to specified path on DUT. Usage: screenshot_func(remote_path).

        @return: An Image object.
                 Notice that the returned image may not be in RGB format,
                 depending on PIL implementation.
        """
        with tempfile.NamedTemporaryFile(suffix='.png') as f:
            basename = os.path.basename(f.name)
            remote_path = os.path.join('/tmp', basename)
            screenshot_func(remote_path)
            self._client.get_file(remote_path, f.name)
            return Image.open(f.name)


    def capture_internal_screen(self):
        """Captures the internal screen framebuffer.

        @return: An Image object.
        """
        screenshot_func = self._display_proxy.take_internal_screenshot
        return self._take_screenshot(screenshot_func)


    # TODO(ihf): This needs to be fixed for multiple external screens.
    def capture_external_screen(self):
        """Captures the external screen framebuffer.

        @return: An Image object.
        """
        screenshot_func = self._display_proxy.take_external_screenshot
        return self._take_screenshot(screenshot_func)


    def get_external_resolution(self):
        """Gets the resolution of the external screen.

        @return The resolution tuple (width, height) or None if no external
                display is connected.
        """
        resolution = self._display_proxy.get_external_resolution()
        return tuple(resolution) if resolution else None


    def get_internal_resolution(self):
        """Gets the resolution of the internal screen.

        @return The resolution tuple (width, height)
        """
        return tuple(self._display_proxy.get_internal_resolution())


    def set_resolution(self, display_index, width, height):
        """Sets the resolution on the specified display.

        @param display_index: index of the display to set resolutions for.
        @param width: width of the resolution
        @param height: height of the resolution
        """
        self._display_proxy.set_resolution(display_index, width, height)


    # pylint: disable = W0141
    def get_display_info(self):
        """Gets the information of all the displays that are connected to the
                DUT.

        @return: list of object DisplayInfo for display informtion
        """
        return map(DisplayInfo, self._display_proxy.get_display_info())


    def get_display_modes(self, display_index):
        """Gets the display modes of the specified display.

        @param display_index: index of the display to get modes from; the index
            is from the DisplayInfo list obtained by get_display_info().

        @return: list of DisplayMode dicts.
        """
        return self._display_proxy.get_display_modes(display_index)


    def get_available_resolutions(self, display_index):
        """Gets the resolutions from the specified display.

        @return a list of (width, height) tuples.
        """
        return [tuple(r) for r in
                self._display_proxy.get_available_resolutions(display_index)]


    def get_first_external_display_index(self):
        """Gets the first external display index.

        @return the index of the first external display; False if not found.
        """
        return self._display_proxy.get_first_external_display_index()
