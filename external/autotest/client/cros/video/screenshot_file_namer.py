# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


from autotest_lib.client.cros.video import method_logger


class ScreenShotFileNamer(object):
    """
    Creates a filename of an image given instance in time it was captured.

    Encapsulates the mapping from a timestamp to a filename.

    There are two uses of this class.

    When downloading golden images, we will have timestamps that we will want
    the images for. Using this class, we will neatly get the filenames from the
    timestamps.

    When capturing test screenshots we will have timestamps also. Again using
    this class we can SAVE the images in the appropriate names. This ensures
    the golden image and test image that are from the same timestamp have
    exactly the same name.

    """


    @method_logger.log
    def __init__(self, image_format):
        self._image_format = image_format


    @property
    def image_format(self):
        """
        Returns the format to use for the image.

        """
        return self._image_format


    @method_logger.log
    def get_filename(self, time_delta_value):
        """
        Gets required full filename for a screenshot image.

        @param time_delta_value: time_delta, the time value at which
        an image will be or has been captured.

        @returns filename encoded based on the time value, as a string.

        """
        hours, remainder = divmod(time_delta_value.total_seconds(), 3600)

        minutes, seconds = divmod(remainder, 60)

        # integer division, discard decimal milliseconds
        milliseconds = time_delta_value.microseconds // 1000

        filename = "%02d_%02d_%02d_%03d" % (
            hours, minutes, seconds, milliseconds)

        return filename + '.' + self.image_format
