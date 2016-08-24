# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os
import time

from autotest_lib.client.bin import site_utils
from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib.cros import chrome
from autotest_lib.client.cros import service_stopper
from autotest_lib.client.cros.graphics import graphics_utils
from autotest_lib.client.cros.image_comparison import rgb_image_comparer
from autotest_lib.client.cros.ui import ui_test_base

class ui_SystemTray(ui_test_base.ui_TestBase):
    """
    Collects system tray screenshots.

    See comments on parent class for overview of how things flow.

    """

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
        Sets the portion of the screenshot to crop.
        Calls into take_screenshot_crop to take the screenshot and crop it.

        self.logged_in controls which logged-in state we are testing when we
        take the screenshot.

        if None, we don't login at all
        if True, we login as the test user
        if False, we login as guest

        For the logged in user we mask the profile photo so that the randomly
        generated profile pictures don't break the tests.

        @param filepath: path, fullpath to where the screenshot will be saved to

        """

        w, h = graphics_utils.get_display_resolution()
        box = (w - self.width, h - self.height, w, h)

        if self.logged_in is None:
            graphics_utils.take_screenshot_crop(filepath, box)
            return

        with chrome.Chrome(logged_in=self.logged_in):
            # set up a pixel comparer
            image_name = os.path.splitext(filepath)[0]
            temp_file_path = '%s_temp.png' % image_name
            comparer = rgb_image_comparer.RGBImageComparer(
                    rgb_pixel_threshold=0)

            def has_animation_stopped():
                """
                Takes two screenshots. Checks if they are identical to
                indicate the shelf has stop animating.

                """

                graphics_utils.take_screenshot_crop(filepath, box)
                graphics_utils.take_screenshot_crop(temp_file_path, box)
                diff = comparer.compare(filepath, temp_file_path)
                logging.debug("Pixel diff count: %d", diff.diff_pixel_count)
                return diff.diff_pixel_count == 0

            # crbug.com/476791 error when take screenshots too soon after login
            time.sleep(30)
            site_utils.poll_for_condition(has_animation_stopped,
                                          timeout=30,
                                          desc='end of system tray animation')

            if self.logged_in and self.mask_points is not None:
                self.draw_image_mask(filepath, self.mask_points)


    def run_once(self, width, height, mask_points=None, logged_in=None):
        self.width = width
        self.height = height
        self.logged_in = logged_in
        self.mask_points = mask_points

        if utils.get_board() != 'link':
            logging.info('This test should only be run on link so exiting.')
            return

        self.run_screenshot_comparison_test()
