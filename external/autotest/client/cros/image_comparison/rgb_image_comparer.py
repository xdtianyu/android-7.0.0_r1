# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import heapq, logging

from PIL import Image
from PIL import ImageChops

from autotest_lib.client.cros.image_comparison import comparison_result
from autotest_lib.client.cros.video import method_logger


"""

*** Consider using PdiffImageComparer instead of this class ***
  * This class uses pixel by pixel comparer while PdiffImageComparer encapsules
  * the perceptualdiff tool available in ChromeOS

"""


class RGBImageComparer(object):
    """
    Compares two RGB images using built-in python image library.

    """


    def __init__(self, rgb_pixel_threshold):
        self.pixel_threshold = rgb_pixel_threshold


    @method_logger.log
    def compare(self, golden_img_path, test_img_path, box=None):
        """
        Compares a test image against a known golden image.

        Both images must be RGB images.

        @param golden_img_path: path, complete path to a golden image.
        @param test_img_path: path, complete path to a test image.
        @param box: int tuple, left, upper, right, lower pixel coordinates
                    defining a box region within which the comparison is made.

        @return: int, number of pixels that are different.

        """
        golden_image = Image.open(golden_img_path)
        test_image = Image.open(test_img_path)

        if golden_image.mode != 'RGB':
            logging.debug('Golden image was not RGB. Converting to RGB.')
            golden_image = golden_image.convert('RGB')

        if test_image.mode != 'RGB':
            logging.debug('Test image was not RGB. Converting to RGB.')
            test_image = test_image.convert('RGB')

        if box is not None:
            golden_image = golden_image.crop(box)
            test_image = test_image.crop(box)

        diff_image = ImageChops.difference(golden_image, test_image)
        maxcolors = diff_image.size[0] * diff_image.size[1]
        colorstuples = diff_image.getcolors(maxcolors)
        max_debug_count = 100

        logging.debug("***ALL Color counts: %d", maxcolors)
        logging.debug(heapq.nlargest(max_debug_count, colorstuples))

        # getcolors returns a list of (count, color) tuples where count is the
        # number of times the corresponding color in the image.

        above_thres_tuples = [t for t in colorstuples
                              if any(pixel > self.pixel_threshold
                                     for pixel in t[1])]

        logging.debug("Color counts above thres.: %d", len(above_thres_tuples))
        logging.debug(heapq.nlargest(max_debug_count, above_thres_tuples))

        diff_pixels = sum(t[0] for t in above_thres_tuples)

        return comparison_result.ComparisonResult(diff_pixels, '')
    
    
    def __enter__(self):
        return self
    
    
    def __exit__(self, exc_type, exc_val, exc_tb):
        pass