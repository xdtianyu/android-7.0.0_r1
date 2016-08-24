# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import ConfigParser

from autotest_lib.client.cros.image_comparison import pdiff_image_comparer
from autotest_lib.client.cros.image_comparison import publisher
from autotest_lib.client.cros.image_comparison import rgb_image_comparer
from autotest_lib.client.cros.image_comparison import verifier
from autotest_lib.client.cros.video import method_logger


class ImageComparisonFactory(object):
    """
    Responsible for instantiating objects used in image comparison based tests.

    """


    def __init__(self, conf_filepath):
        """
        @param conf_filepath: path, full path to the conf file.

        """
        self.conf_filepath = conf_filepath
        self._load_configuration()


    def _load_configuration(self):
        """
        Loads values from configuration file.

        """
        parser = ConfigParser.SafeConfigParser()
        parser.read(self.conf_filepath)

        self.pixel_thres = parser.getint('rgb', 'rgb_pixel_threshold')
        self.pixel_count_thres = parser.getint('all', 'pixel_count_threshold')
        self.desired_comp_h = parser.getint('all', 'desired_comp_h')
        self.desired_comp_w = parser.getint('all', 'desired_comp_w')


    @method_logger.log
    def make_rgb_comparer(self):
        """
        @returns an RGBImageComparer object initialized with config. values.

        """
        return rgb_image_comparer.RGBImageComparer(self.pixel_thres)


    @method_logger.log
    def make_pdiff_comparer(self):
        """
        @returns a PDiffImageComparer object.

        """
        return pdiff_image_comparer.PdiffImageComparer()


    @method_logger.log
    def make_image_verifier(self, image_comparer, stop_on_first_failure=False):
        """
        @param image_comparer: any object that implements compare(). Currently,
                               it could RGBImageComparer or
                               UploadOnFailComparer.

        @param stop_on_first_failure: bool, True if we should stop the test when
                                      we encounter the first failed comparison.
                                      False if we should continue the test.
        @returns a Verifier object initialized with config. values.

        """
        if self.desired_comp_h == 0 or self.desired_comp_w == 0:
            box = None
        else:
            box = (0, 0, self.desired_comp_w, self.desired_comp_h)

        return verifier.Verifier(image_comparer,
                                 stop_on_first_failure,
                                 threshold=self.pixel_count_thres,
                                 box=box)


    @method_logger.log
    def make_imagediff_publisher(self, results_folder):
        """
        @param results_folder: path, where to publish the results to
        @returns an ImageDIffPublisher object

        """
        return publisher.ImageDiffPublisher(results_folder)