# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, os, time

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import chrome, enrollment

TIMEOUT = 20

class enterprise_RemoraRequisition(test.test):
    """Enroll as a Remora device."""
    version = 1

    _HANGOUTS_EXT_ID = 'acdafoiapclbpdkhnighhilgampkglpc'

    def _WaitForHangouts(self, browser):
        def _HangoutExtContexts():
            try:
                ext_contexts = browser.extensions.GetByExtensionId(
                        self._HANGOUTS_EXT_ID)
                if len(ext_contexts) > 1:
                    return ext_contexts
            except (KeyError, chrome.Error):
                pass
            return []
        return utils.poll_for_condition(
                _HangoutExtContexts,
                exception=error.TestFail('Hangouts app failed to launch'),
                timeout=30,
                sleep_interval=1)

    def _CheckHangoutsExtensionContexts(self, browser):
        ext_contexts = self._WaitForHangouts(browser)
        ext_urls = set([context.EvaluateJavaScript('location.href;')
                       for context in ext_contexts])
        expected_urls = set(
                ['chrome-extension://' + self._HANGOUTS_EXT_ID + '/' + path
                for path in ['hangoutswindow.html?windowid=0',
                             '_generated_background_page.html']])
        if expected_urls != ext_urls:
            raise error.TestFail(
                    'Unexpected extension context urls, expected %s, got %s'
                    % (expected_urls, ext_urls))


    def run_once(self):
        user_id, password = utils.get_signin_credentials(os.path.join(
                os.path.dirname(os.path.realpath(__file__)), 'credentials.txt'))
        if not (user_id and password):
            logging.warn('No credentials found - exiting test.')
            return

        with chrome.Chrome(auto_login=False) as cr:
            enrollment.RemoraEnrollment(cr.browser, user_id, password)
            # Timeout to allow for the device to stablize and go back to the
            # login screen before proceeding.
            time.sleep(TIMEOUT)

        # This is a workaround fix for crbug.com/495847. A more permanent fix
        # should be to get the hotrod app to auto launch after enrollment.
        with chrome.Chrome(clear_enterprise_policy=False,
                           dont_override_profile=True,
                           disable_gaia_services=False,
                           disable_default_apps=False,
                           auto_login=False) as cr:
            self._CheckHangoutsExtensionContexts(cr.browser)
