# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import errno
import Image
import logging
import subprocess
import tempfile

from autotest_lib.client.cros.image_comparison import comparison_result
from autotest_lib.client.cros.video import method_logger


class PdiffImageComparer(object):
    """
    Compares two images using ChromeOS' perceptualdiff binary.

    """

    @method_logger.log
    def compare(self, golden_img_path, test_img_path, box=None):
        """
        Compares a test image against the specified golden image using the
        terminal tool 'perceptualdiff'.

        @param golden_img_path: path, complete path to a golden image.
        @param test_img_path: path, complete path to a test image.
        @param box: int tuple, left, upper, right, lower pixel coordinates.
                    Defines the rectangular boundary within which to compare.
        @return: int, number of pixels that are different.
        @raise : Whatever _pdiff_compare raises.

        """
        if not box:
            return self._pdiff_compare(golden_img_path, test_img_path)

        ext = '.png'
        tmp_golden_img_file = tempfile.NamedTemporaryFile(suffix=ext)
        tmp_test_img_file = tempfile.NamedTemporaryFile(suffix=ext)

        with tmp_golden_img_file, tmp_test_img_file:
            tmp_golden_img_path = tmp_golden_img_file.name
            tmp_test_img_path = tmp_test_img_file.name

            Image.open(golden_img_path).crop(box).save(tmp_golden_img_path)
            Image.open(test_img_path).crop(box).save(tmp_test_img_path)

            return self._pdiff_compare(tmp_golden_img_path, tmp_test_img_path)


    def _pdiff_compare(self, golden_img_path, test_img_path):
        """
        Invokes perceptualdiff using subprocess tools.

        @param golden_img_path: path, complete path to a golden image.
        @param test_img_path: path, complete path to a test image.
        @return: int, number of pixels that are different.
        @raise ValueError if image dimensions are not the same.
        @raise OSError: if file does not exist or can not be opened.

        """

        # TODO mussa: Could downsampling the images be good for us?

        tmp_diff_file = tempfile.NamedTemporaryFile(suffix='.png', delete=False)
        args = ['perceptualdiff', golden_img_path, test_img_path, '-output',
                tmp_diff_file.name]

        logging.debug("Start process with args : " + str(args))

        p = subprocess.Popen(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

        output = p.communicate()
        logging.debug('output of perceptual diff command is')
        logging.debug(output)

        stdoutdata = output[0]

        mismatch_error = "Image dimensions do not match"
        diff_message = "Images are visibly different"
        filenotfound_message = "Cannot open"

        #TODO(dhaddock): Check for image not created
        if p.returncode == 0:
            # pdiff exited with 0, images were the same
            return comparison_result.ComparisonResult(0, '', None)

        if mismatch_error in stdoutdata:
            raise ValueError("pdiff says: " + stdoutdata)

        if diff_message in stdoutdata:
            diff_pixels = [int(s) for s in stdoutdata.split() if s.isdigit()][0]
            return comparison_result.ComparisonResult(int(diff_pixels), '',
                                                      tmp_diff_file.name)

        if filenotfound_message in stdoutdata:
            raise OSError(errno.ENOENT, "pdiff says: " + stdoutdata)

        raise RuntimeError("Unknown result from pdiff: "
                           "Output : " + stdoutdata)

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        pass