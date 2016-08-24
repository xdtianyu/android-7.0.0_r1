# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib.cros import chrome
from autotest_lib.client.cros.cellular import test_environment
from autotest_lib.client.cros.ui import ui_test_base
from autotest_lib.client.common_lib import error
from telemetry.util import image_util

class ui_SettingsPage(ui_test_base.ui_TestBase):
    """
    Collects screenshots of the settings page.
    See comments on parent class for overview of how things flow.

    """

    def capture_screenshot(self, filepath):
        """
        Takes a screenshot of the settings page.

        A mask is then drawn over the profile picture. This test runs only
        on link at the moment so the dimensions provided are link specific.

        Implements the abstract method capture_screenshot.

        @param filepath: string, complete path to save screenshot to.

        """
        with chrome.Chrome() as cr:
            tab = cr.browser.tabs[0]
            tab.Navigate('chrome://settings')
            tab.WaitForDocumentReadyStateToBeComplete()

            if not tab.screenshot_supported:
                raise error.TestError('Tab did not support taking screenshots')

            screenshot = tab.Screenshot()
            if screenshot is None:
                raise error.TestFailure('Could not capture screenshot')

            image_util.WritePngFile(screenshot, filepath)

    def run_once(self, mask_points):
        # Emulate a modem on the device.
        test_env = test_environment.CellularPseudoMMTestEnvironment(
                pseudomm_args=({'family': '3GPP'},),
                use_backchannel=False,
                shutdown_other_devices=False)

        with test_env:
            self.mask_points = mask_points

            # Check if we should find mobile data in settings
            # This should always return true now, since the modem is software
            # emulated.
            modem_status = utils.system_output('modem status')
            if modem_status:
                logging.info('Modem found')
                logging.info(modem_status)
                self.tagged_testname += '.mobile'
            else:
                logging.info('Modem not found')
            self.run_screenshot_comparison_test()
