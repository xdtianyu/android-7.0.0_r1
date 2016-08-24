# Copyright (c) 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os

from autotest_lib.client.common_lib.cros import chrome
from autotest_lib.client.cros import service_stopper
from autotest_lib.client.cros.input_playback import input_playback
from autotest_lib.client.cros.graphics import graphics_utils
from autotest_lib.client.cros.ui import ui_test_base


class ui_AppLauncher(ui_test_base.ui_TestBase):
    """
    Collects screenshots of the App Launcher.
    See comments on parent class for overview of how things flow.

    """

    # The keyboard we are emulating
    _KEYBOARD_PROP = 'keyboard.prop'

    # The keyboard playback data
    _KEYBOARD_PLAYBACK = 'searchkey_tabs_enter'

    def initialize(self):
        """Perform necessary initialization prior to test run.

        Private Attributes:
          _services: service_stopper.ServiceStopper object
        """
        # Do not switch off screen for screenshot utility.
        self._services = service_stopper.ServiceStopper(['powerd'])
        self._services.stop_services()

    def cleanup(self):
        self._services.restore_services()

    def capture_screenshot(self, filepath):
        """
        Take a screenshot of the App Launcher page.

        Implements the abstract method capture_screenshot

        @param filepath: string, Complete path to save the screenshot to.

        """

        # Login and load the default apps
        with chrome.Chrome(disable_default_apps=False):

            # Setup the keyboard file's paths
            property_file = os.path.join(self.bindir, self._KEYBOARD_PROP)
            playback_file = os.path.join(self.bindir, self._KEYBOARD_PLAYBACK)

            # Setup and playback the keyboard commands to open the launcher
            player = input_playback.InputPlayback()
            player.emulate('keyboard', property_file)
            player.find_connected_inputs()
            player.blocking_playback(playback_file, 'keyboard')
            player.close()

            # Take a screenshot and crop to just the launcher
            w, h = graphics_utils.get_internal_resolution()
            upper_x = (w - self.launcher_width) / 2
            upper_y = (h - self.launcher_height) / 2
            box = (upper_x, upper_y, upper_x + self.launcher_width, upper_y +
                   self.launcher_height)

            graphics_utils.take_screenshot_crop(filepath, box)

    def run_once(self):
        # The default launcher dimensions
        self.launcher_width = 768
        self.launcher_height = 570

        w, h = graphics_utils.get_internal_resolution()
        logging.info('DUT screen width: %d' % w)
        logging.info('DUT screen height: %d' % h)

        # If we have a high DPI screen, launcher size is doubled
        if self.launcher_width * 2 < w:
            self.launcher_width *= 2
            self.launcher_height *= 2
            self.tagged_testname += '.large'

        self.run_screenshot_comparison_test()
