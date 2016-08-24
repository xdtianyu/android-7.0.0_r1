# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import abc
import datetime
import os
import urllib2

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error, file_utils, lsbrelease_utils
from autotest_lib.client.cros import constants
from autotest_lib.client.cros.image_comparison import image_comparison_factory
from PIL import Image
from PIL import ImageDraw

class ui_TestBase(test.test):
    """ Encapsulates steps needed to collect screenshots for ui pieces.

    Each child class must implement:
    1. Abstract method capture_screenshot()
    Each child class will define its own custom way of capturing the screenshot
    of the piece it cares about.

    E.g Child class ui_SystemTray will capture system tray screenshot,
    ui_SettingsPage for the Chrome Settings page, etc.

    2. Abstract property test_area:
    This will get appended to screenshot file names so we know what image it is.

    Flow at runtime:
    At run time, autotest will call run_once() method on a particular child
    class object, call it Y.

    Say X is a parent of Y.

    Y.run_once() will save any values passed from control file so as to use them
    later.

    Y.run_once() will then call the parent's X.run_screenshot_comparison_test()

    This is the template algorithm for collecting screenshots.

    Y.run_screenshot_comparison_test will execute its steps. It will then call
    X.test_area to get custom string to use for project name and filename.

     It will execute more steps and then call capture_screenshot(). X doesn't
     implement that, but Y does, so the method will get called on Y to produce
     Y's custom behavior.

     Control will be returned to Y run_screenshot_comparison_test() which will
     execute remainder steps.

    """

    __metaclass__ = abc.ABCMeta

    WORKING_DIR = '/tmp/test'
    REMOTE_DIR = 'http://storage.googleapis.com/chromiumos-test-assets-public'
    AUTOTEST_CROS_UI_DIR = '/usr/local/autotest/cros/ui'
    IMG_COMP_CONF_FILE = 'image_comparison.conf'

    version = 2


    def run_screenshot_comparison_test(self):
        """
        Template method to run screenshot comparison tests for ui pieces.

        1. Set up test dirs.
        2. Create folder name
        3. Download golden image.
        4. Capture test image.
        5. Compare images locally, if FAIL upload to remote for analysis later.
        6. Clean up test dirs.

        """

        img_comp_conf_path = os.path.join(ui_TestBase.AUTOTEST_CROS_UI_DIR,
                                          ui_TestBase.IMG_COMP_CONF_FILE)

        img_comp_factory = image_comparison_factory.ImageComparisonFactory(
                img_comp_conf_path)

        golden_image_local_dir = os.path.join(ui_TestBase.WORKING_DIR,
                                              'golden_images')

        file_utils.make_leaf_dir(golden_image_local_dir)

        filename = '%s.png' % self.tagged_testname

        golden_image_remote_path = os.path.join(
                ui_TestBase.REMOTE_DIR,
                'ui',
                lsbrelease_utils.get_chrome_milestone(),
                self.folder_name,
                filename)

        golden_image_local_path = os.path.join(golden_image_local_dir, filename)

        test_image_filepath = os.path.join(ui_TestBase.WORKING_DIR, filename)

        try:
            file_utils.download_file(golden_image_remote_path,
                                     golden_image_local_path)
        except urllib2.HTTPError as e:
            warn = "No screenshot found for {0} on milestone {1}. ".format(
                self.tagged_testname, lsbrelease_utils.get_chrome_milestone())
            warn += e.msg
            raise error.TestWarn(warn)

        self.capture_screenshot(test_image_filepath)



        comparer = img_comp_factory.make_pdiff_comparer()
        comp_res = comparer.compare(golden_image_local_path,
                                    test_image_filepath)

        if comp_res.diff_pixel_count > img_comp_factory.pixel_thres:
            publisher = img_comp_factory.make_imagediff_publisher(
                    self.resultsdir)

            # get chrome version
            version_string = utils.system_output(
                constants.CHROME_VERSION_COMMAND, ignore_status=True)
            version_string = utils.parse_chrome_version(version_string)[0]

            # tags for publishing
            tags = {
                'testname': self.tagged_testname,
                'chromeos_version': utils.get_chromeos_release_version(),
                'chrome_version': version_string,
                'board':  utils.get_board(),
                'date': datetime.date.today().strftime("%m/%d/%y"),
                'diff_pixels': comp_res.diff_pixel_count
            }

            publisher.publish(golden_image_local_path,
                                    test_image_filepath,
                                    comp_res.pdiff_image_path, tags)

            raise error.TestFail('Test Failed. Please see image comparison '
                                 'result by opening index.html from the '
                                 'results directory.')

        file_utils.rm_dir_if_exists(ui_TestBase.WORKING_DIR)


    @property
    def folder_name(self):
        """
        Computes the folder name to look for golden images in
        based on the current test area.

        If we have tagged our testcase, it removes the tag to
        get the base testname.

        E.g if we add the tag 'guest' to the ui_SystemTray class,
        the tagged test name will be ui_SystemTray.guest

        This removes the tag if it was added
        """

        return self.tagged_testname.split('.')[0]


    @abc.abstractmethod
    def capture_screenshot(self, filepath):
        """
        Abstract method to capture a screenshot.
        Child classes must implement a custom way to take screenshots.
        This is because each will want to crop to different areas of the screen.

        @param filepath: string, complete path to save the screenshot.

        """
        pass

    def draw_image_mask(self, filepath, rectangle, fill='white'):
        """
        Used to draw a mask over selected portions of the captured screenshot.
        This allows us to mask out things that change between runs while
        letting us focus on the parts we do care about.

        @param filepath: string, the complete path to the image
        @param rectangle: tuple, the top left and bottom right coordinates
        @param fill: string, the color to fill the mask with

        """

        im = Image.open(filepath)
        draw = ImageDraw.Draw(im)
        draw.rectangle(rectangle, fill=fill)
        im.save(filepath)
