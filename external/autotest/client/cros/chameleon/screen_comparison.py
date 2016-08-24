# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Classes to do screen comparison."""

import logging
import os
import time

from PIL import ImageChops


class ScreenComparer(object):
    """A class to compare two screens.

    Calling its member method compare() does the comparison.

    """

    def __init__(self, capturer1, capturer2, output_dir, pixel_diff_margin,
                 wrong_pixels_margin):
        """Initializes the ScreenComparer objects.

        @param capture1: The screen capturer object.
        @param capture2: The screen capturer object.
        @param output_dir: The directory for output images.
        @param pixel_diff_margin: The margin for comparing a pixel. Only
                if a pixel difference exceeds this margin, will treat as a wrong
                pixel. Sets None means using default value by detecting
                connector type.
        @param wrong_pixels_margin: The percentage of margin for wrong pixels.
                The value is in a closed interval [0.0, 1.0]. If the total
                number of wrong pixels exceeds this margin, the check fails.
        """
        # TODO(waihong): Support multiple capturers.
        self._capturer1 = capturer1
        self._capturer2 = capturer2
        self._output_dir = output_dir
        self._pixel_diff_margin = pixel_diff_margin
        assert 0.0 <= wrong_pixels_margin <= 1.0
        self._wrong_pixels_margin = wrong_pixels_margin


    def compare(self):
        """Compares the screens.

        @return: None if the check passes; otherwise, a string of error message.
        """
        tags = [self._capturer1.TAG, self._capturer2.TAG]
        images = [self._capturer1.capture(), self._capturer2.capture()]

        if None in images:
            message = ('Failed to capture the screen of %s.' %
                       tags[images.index(None)])
            logging.error(message)
            return message

        # Sometimes the format of images got from X is not RGB,
        # which may lead to ValueError raised by ImageChops.difference().
        # So here we check the format before comparing them.
        for i, image in enumerate(images):
          if image.mode != 'RGB':
            images[i] = image.convert('RGB')

        message = 'Unexpected exception'
        time_str = time.strftime('%H%M%S')
        try:
            # The size property is the resolution of the image.
            if images[0].size != images[1].size:
                message = ('Sizes of images %s and %s do not match: '
                           '%dx%d != %dx%d' %
                           (tuple(tags) + images[0].size + images[1].size))
                logging.error(message)
                return message

            size = images[0].size[0] * images[0].size[1]
            max_acceptable_wrong_pixels = int(self._wrong_pixels_margin * size)

            logging.info('Comparing the images between %s and %s...', *tags)
            diff_image = ImageChops.difference(*images)
            histogram = diff_image.convert('L').histogram()

            num_wrong_pixels = sum(histogram[self._pixel_diff_margin + 1:])
            max_diff_value = max(filter(
                    lambda x: histogram[x], xrange(len(histogram))))
            if num_wrong_pixels > 0:
                logging.debug('Histogram of difference: %r', histogram)
                prefix_str = '%s-%dx%d' % ((time_str,) + images[0].size)
                message = ('Result of %s: total %d wrong pixels '
                           '(diff up to %d)' % (
                           prefix_str, num_wrong_pixels, max_diff_value))
                if num_wrong_pixels > max_acceptable_wrong_pixels:
                    logging.error(message)
                    return message

                message += (', within the acceptable range %d' %
                            max_acceptable_wrong_pixels)
                logging.warning(message)
            else:
                logging.info('Result: all pixels match (within +/- %d)',
                             max_diff_value)
            message = None
            return None
        finally:
            if message is not None:
                for i in (0, 1):
                    # Use time and image size as the filename prefix.
                    prefix_str = '%s-%dx%d' % ((time_str,) + images[i].size)
                    # TODO(waihong): Save to a better lossless format.
                    file_path = os.path.join(
                            self._output_dir,
                            '%s-%s.png' % (prefix_str, tags[i]))
                    logging.info('Output the image %d to %s', i, file_path)
                    images[i].save(file_path)

                file_path = os.path.join(
                        self._output_dir, '%s-diff.png' % prefix_str)
                logging.info('Output the diff image to %s', file_path)
                diff_image = ImageChops.difference(*images)
                gray_image = diff_image.convert('L')
                bw_image = gray_image.point(
                        lambda x: 0 if x <= self._pixel_diff_margin else 255,
                        '1')
                bw_image.save(file_path)
