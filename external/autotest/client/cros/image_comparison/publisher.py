# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import base64
import datetime
import glob
import os
import shutil

from autotest_lib.client.bin import utils
from autotest_lib.client.cros import constants
from string import Template

class ImageDiffPublisher(object):
    """
    Class that takes care of creating the HTML file output when a pdiff
    comparison fails. It moves each of the three images to a folder in the
    results directory. It then writes a html file that references these images.

    """

    VIEWER_FILES = '/usr/local/autotest/cros/image_comparison/diffviewer/*'


    def __init__(self, results_folder):
        """
        @param results_folder: path, where to publish to
        """
        self.results_folder = results_folder
        # Copy files needed to the results dir
        for diff_viewer_file in glob.glob(self.VIEWER_FILES):
            shutil.copy(diff_viewer_file, self.results_folder)


    def publish(self, golden_image_path, test_image_path, diff_image_path,
                tags):
        """
        Move viewer files to the results folder and base64 encode the images.
        Write tags to HTML file.

        @param golden_image_path: path, complete path to a golden image.
        @param test_image_path: path, complete path to a test image.
        @param diff_image_path: path, complete path to a diff image.
        @param tags: list, run information.
        """

        # Encode the images to base64
        base64_images = {}
        with open(golden_image_path, "rb") as image_file:
            base64_images["golden"] = base64.b64encode(image_file.read())
        with open(test_image_path, "rb") as image_file:
            base64_images["test"] = base64.b64encode(image_file.read())
        with open(diff_image_path, "rb") as image_file:
            base64_images["diff"] = base64.b64encode(image_file.read())

        # Append all of the things we push to the html template
        tags.update(base64_images)

        html_file_fullpath = os.path.join(self.results_folder, 'index.html')
        self._write_tags_to_html(tags, html_file_fullpath)


    def publish_paths(self, image_paths, testname):
        """
        Creates a results page for an array of images.

        Move viewer files to the results folder and base64 encode the images.
        Write tags to HTML file.

        @param image_paths: an array of paths
        @param testname: name of current test.
        """

        img_tags = []
        for img in image_paths:
            with open(img, "rb") as image_file:
                b64img = base64.b64encode(image_file.read())
                b64imgsrc = "data:image/png;base64, " + b64img
                img_tags.append(b64imgsrc)

        tags = self._generate_tags(testname)
        tags['images'] = img_tags
        html_file_fullpath = os.path.join(self.results_folder, 'slideshow.html')
        self._write_tags_to_html(tags, html_file_fullpath)


    def _write_tags_to_html(self, tags, html_filename):
        """
        Writes tags to the HTML file

        @param tags the tags to write into the html template
        @param html_filename the full path to the html template
        """

        with open(html_filename, 'r+') as f:
            html = Template(f.read())
            formatted_html = html.substitute(tags)
            f.seek(0)
            f.write(formatted_html)


    def _generate_tags(self, testname):
        """
        Generate tags for the current test run

        @param testname the name of the current test
        @return an array of tags
        """
        # get chrome version
        version_string = utils.system_output(
            constants.CHROME_VERSION_COMMAND, ignore_status=True)
        version_string = utils.parse_chrome_version(version_string)[0]

        return {
            'testname': testname,
            'chromeos_version': utils.get_chromeos_release_version(),
            'chrome_version': version_string,
            'board': utils.get_board(),
            'date': datetime.date.today().strftime("%m/%d/%y"),
        }
