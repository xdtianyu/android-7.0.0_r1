# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import logging
import time

from autotest_lib.client.bin import test
from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import chrome


class accessibility_Sanity(test.test):
    """Enables then disables all a11y features via accessibilityFeatures API."""
    version = 1

    # Features that do not have their own separate tests
    _FEATURE_LIST = [
        'largeCursor',
        'stickyKeys',
        'highContrast',
        'screenMagnifier',
        'autoclick',
        'virtualKeyboard'
    ]

    # ChromeVox extension id
    _CHROMEVOX_ID = 'mndnfokpggljbaajbnioimlmbfngpief'

    def _set_feature(self, feature, value):
        """Set given feature to given value using a11y API call.

            @param feature: string of accessibility feature to change
            @param value: boolean of expected value
        """
        value_str = 'true' if value else 'false'
        cmd = '''
            window.__result = null;
            chrome.accessibilityFeatures.%s.set({value: %s});
            chrome.accessibilityFeatures.%s.get({}, function(d) {
                window.__result = d[\'value\'];
            });
        ''' % (feature, value_str, feature)
        self._extension.ExecuteJavaScript(cmd)

        poll_cmd = 'window.__result == %s;' % value_str
        utils.poll_for_condition(
                lambda: self._extension.EvaluateJavaScript(poll_cmd),
                exception = error.TestError(
                        'Timeout while trying to set %s to %s' %
                        (feature, value_str)))

    def _confirm_chromevox_indicator(self, value):
        """Fail test unless indicator presence is given value on self._tab."""
        poll_cmd = '''
            document.getElementsByClassName("cvox_indicator_container").length;
        '''
        def _poll_function():
            if value:
                return self._tab.EvaluateJavaScript(poll_cmd) > 0
            else:
                return self._tab.EvaluateJavaScript(poll_cmd) == 0

        utils.poll_for_condition(
                _poll_function,
                exception=error.TestError('ChromeVox: "Indicator present" '
                                          'was not %s.' % value))

    def _confirm_chromevox_enabled(self, value):
        """Fail test unless management.get.enabled is given value."""
        cmd = '''
            window.__enabled = false;
            chrome.management.get(
                    '%s', function(r) {window.__enabled = r[\'enabled\']});
        ''' % self._CHROMEVOX_ID
        self._extension.ExecuteJavaScript(cmd)

        poll_cmd = 'window.__enabled;'
        utils.poll_for_condition(
                lambda: self._extension.EvaluateJavaScript(poll_cmd) == value,
                exception=error.TestError(
                        'ChromeVox: management.get.enabled not %s.' % value))

    def _check_chromevox(self):
        """Run ChromeVox specific checks.

            Check result of management.get.enabled before/after enable and
            for presence of indicator before/after disable.
        """
        # Check for ChromeVox running in the background.
        self._confirm_chromevox_enabled(False)
        self._set_feature('spokenFeedback', True)
        time.sleep(1)
        self._confirm_chromevox_enabled(True)

        # Check for presence of ChromeVox indicators.
        self._confirm_chromevox_indicator(True)
        self._set_feature('spokenFeedback', False)
        self._tab.Navigate(self._url) # reload page to remove old indicators
        self._confirm_chromevox_indicator(False)

    def run_once(self):
        """Entry point of this test."""
        extension_path = os.path.join(os.path.dirname(__file__), 'a11y_ext')

        with chrome.Chrome(extension_paths=[extension_path],
                           is_component=False) as cr:
            self._extension = cr.get_extension(extension_path)

            # Open test page.
            self._tab = cr.browser.tabs[0]
            cr.browser.platform.SetHTTPServerDirectories(
                    os.path.join(os.path.dirname(__file__)))
            page_path = os.path.join(self.bindir, 'page.html')
            self._url = cr.browser.platform.http_server.UrlOf(page_path)
            self._tab.Navigate(self._url)

            # Check specific features.
            self._check_chromevox()

            # Enable then disable all other accessibility features.
            for value in [True, False]:
                for feature in self._FEATURE_LIST:
                    logging.info('Setting %s to %s.', feature, value)
                    self._set_feature(feature, value)
                    time.sleep(1)
