# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
import glob, logging, os

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import base_utils, error
from autotest_lib.client.cros.graphics import graphics_utils

def get_percent_difference(file1, file2):
    """
    Performs byte-by-byte comparison of two files, given by their paths |file1|
    and |file2|.  Returns difference as a percentage of the total file size.  If
    one file is larger than the other, the difference is a percentage of
    |file1|.
    """
    files = (file1, file2)
    sizes = {}
    for filename in files:
        if not os.path.exists(filename):
            raise error.TestFail('Could not find file \'%s\'.' % filename)
        sizes[filename] = os.path.getsize(filename)
        if sizes[filename] == 0:
            raise error.TestFail('File \'%s\' has zero size.' % filename)

    diff_bytes = int(utils.system_output('cmp -l %s %s | wc -l' % files))

    return round(100. * diff_bytes / sizes[file1])


class graphics_VTSwitch(test.test):
    """
    Verify that VT switching works.
    """
    version = 1
    GSC = None
    # TODO(crosbug.com/36417): Need to handle more than one display screen.

    def setup(self):
        self.job.setup_dep(['gfxtest'])

    def initialize(self):
        self.GSC = graphics_utils.GraphicsStateChecker()

    def cleanup(self):
        # Return to VT1 when done.  Ideally, the screen should already be in VT1
        # but the test might fail and terminate while in VT2.
        self._switch_to_vt(1)
        if self.GSC:
            self.GSC.finalize()

    def run_once(self,
                 num_iterations=2,
                 similarity_percent_threshold=95,
                 difference_percent_threshold=5):
        # TODO(ihf): Remove this once VTSwitch works on freon.
        if utils.is_freon():
            raise error.TestNAError(
                    'Test needs work on Freon. See crbug.com/413088.')

        self._num_errors = 0
        keyvals = {}

        # Make sure we start in VT1
        if not self._switch_to_vt(1):
            raise error.TestFail('Could not switch to VT1')

        # Take screenshot of sign-in screen.
        logged_out_screenshot = self._take_current_vt_screenshot()

        keyvals['num_iterations'] = num_iterations

        # Go to VT2 and take a screenshot.
        if not self._switch_to_vt(2):
            raise error.TestFail('Could not switch to VT2')
        vt2_screenshot = self._take_current_vt_screenshot()

        # Make sure VT1 and VT2 are sufficiently different.
        diff = get_percent_difference(logged_out_screenshot, vt2_screenshot)
        keyvals['percent_initial_VT1_VT2_difference'] = diff
        if not diff >= difference_percent_threshold:
            self._num_errors += 1
            logging.error('VT1 and VT2 screenshots only differ by ' + \
                          '%d %%: %s vs %s' %
                          (diff, logged_out_screenshot, vt2_screenshot))

        num_identical_vt1_screenshots = 0
        num_identical_vt2_screenshots = 0
        max_vt1_difference_percent = 0
        max_vt2_difference_percent = 0

        # Repeatedly switch between VT1 and VT2.
        for iteration in xrange(num_iterations):
            logging.info('Iteration #%d', iteration)

            # Go to VT1 and take a screenshot.
            self._switch_to_vt(1)
            current_vt1_screenshot = self._take_current_vt_screenshot()

            # Make sure the current VT1 screenshot is the same as (or similar
            # to) the original login screen screenshot.
            diff = get_percent_difference(logged_out_screenshot,
                                          current_vt1_screenshot)
            if not diff < similarity_percent_threshold:
                max_vt1_difference_percent = \
                    max(diff, max_vt1_difference_percent)
                self._num_errors += 1
                logging.error('VT1 screenshots differ by %d %%: %s vs %s',
                              diff, logged_out_screenshot,
                              current_vt1_screenshot)
            else:
                num_identical_vt1_screenshots += 1

            # Go to VT2 and take a screenshot.
            self._switch_to_vt(2)
            current_vt2_screenshot = self._take_current_vt_screenshot()

            # Make sure the current VT2 screenshot is the same as (or similar
            # to) the first VT2 screenshot.
            diff = get_percent_difference(vt2_screenshot,
                                          current_vt2_screenshot)
            if not diff <= similarity_percent_threshold:
                max_vt2_difference_percent = \
                    max(diff, max_vt2_difference_percent)
                self._num_errors += 1
                logging.error(
                    'VT2 screenshots differ by %d %%: %s vs %s',
                    diff, vt2_screenshot, current_vt2_screenshot)
            else:
                num_identical_vt2_screenshots += 1

        self._switch_to_vt(1)

        keyvals['percent_VT1_screenshot_max_difference'] = \
            max_vt1_difference_percent
        keyvals['percent_VT2_screenshot_max_difference'] = \
            max_vt2_difference_percent
        keyvals['num_identical_vt1_screenshots'] = num_identical_vt1_screenshots
        keyvals['num_identical_vt2_screenshots'] = num_identical_vt2_screenshots

        self.write_perf_keyval(keyvals)

        if self._num_errors > 0:
            raise error.TestError('Test failed with %d errors' %
                                  self._num_errors)


    def _take_current_vt_screenshot(self):
        """
        Captures a screenshot of the current VT screen in BMP format.
        Returns the path of the screenshot file.
        """
        current_vt = int(utils.system_output('fgconsole'))
        extension = 'bmp'

        # In VT1, X is running so use that screenshot function.
        if current_vt == 1:
            return graphics_utils.take_screenshot(self.resultsdir,
                                                  'graphics_VTSwitch_VT1',
                                                  extension)

        # Otherwise, grab the framebuffer using DRM.
        prefix = 'graphics_VTSwitch_VT2'
        next_index = len(glob.glob(
            os.path.join(self.resultsdir, '%s-*.%s' % (prefix, extension))))
        filename = '%s-%d.%s' % (prefix, next_index, extension)
        output_path = os.path.join(self.resultsdir, filename)
        return self._take_drm_screenshot(output_path)


    def _take_drm_screenshot(self, output_path):
        """
        Takes drm screenshot.
        """
        autotest_deps_path = os.path.join(self.autodir, 'deps')
        getfb_path = os.path.join(autotest_deps_path, 'gfxtest', 'getfb')
        output = utils.system_output('%s %s.rgba' % (getfb_path, output_path))
        for line in output.split('\n'):
            # Parse the getfb output for info about framebuffer size.  The line
            # should looks omething like:
            #   Framebuffer info: 1024x768, 32bpp
            if line.startswith('Framebuffer info:'):
                size = line.split(':')[1].split(',')[0].strip()
                break
        utils.system('convert -depth 8 -size %s %s.rgba %s' %
                     (size, output_path, output_path))

        logging.info('Saving screenshot to %s', output_path)
        return output_path


    def _switch_to_vt(self, vt):
        """
        Switches to virtual terminal given by |vt| (1, 2, etc) and checks that
        the switch was successful by calling fgconsole.

        Returns True if fgconsole returned the new vt number, False otherwise.
        """
        utils.system_output('chvt %d' % vt)

        # Verify that the VT switch was successful.
        current_vt = base_utils.wait_for_value(
            lambda: int(utils.system_output('fgconsole')),
            expected_value=vt)
        if vt != current_vt:
            self._num_errors += 1
            logging.error('Current VT %d does not match expected VT %d',
                          current_vt, vt)
            return False
        logging.info('Switched to VT%d', vt)
        return True
