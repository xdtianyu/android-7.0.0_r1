# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os

from autotest_lib.client.cros.graphics import graphics_utils
from autotest_lib.client.cros.video import method_logger


class ImportScreenShotCapturer(object):
    """
    Captures a screenshot with the required dimensions from a chromebook.

    Uses utility capture but specifies the geometry/dimensions of final image.

    We need this so that we can chop off things like browser address bar and
    system task bar that are not only irrelevant to the test but would also add
    noise to the test.

    """


    @method_logger.log
    def __init__(self, destination_dir, screen_height_resolution,
                 top_pixels_to_crop, bottom_pixels_to_crop):
        self.destination_dir = destination_dir
        self.screen_height_resolution = screen_height_resolution
        self.top_pixels_to_crop = top_pixels_to_crop
        self.bottom_pixels_to_crop = bottom_pixels_to_crop


    def __enter__(self):
        return self


    @method_logger.log
    def capture(self, filename):
        """
        Capture the screenshot.

        Use pre-configured information to create a geometry that specifies the
        final dimension and position of the image.

        @param filename: string, the screenshot filename.

        @returns a complete path to the screenshot generated.

        """
        fullpath = os.path.join(self.destination_dir, filename)

        final_height = (self.screen_height_resolution -
                        self.top_pixels_to_crop - self.bottom_pixels_to_crop)

        graphics_utils.take_screenshot_crop_by_height(fullpath,
                                                      final_height,
                                                      0,
                                                      self.top_pixels_to_crop)

        return fullpath


    def __exit__(self, exc_type, exc_val, exc_tb):
        pass
