# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import re

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import chrome
from telemetry.core import exceptions

class security_SandboxStatus(test.test):
    """Verify sandbox status."""
    version = 1


    def _EvaluateJavaScript(self, js):
        '''Evaluates js, returns None if an exception was thrown.'''

        try:
            return self._tab.EvaluateJavaScript(js)
        except exceptions.EvaluateException:
            return None

    def _CheckSandboxPage(self, url, js):
        self._tab.Navigate(url)

        return utils.poll_for_condition(
                lambda: self._EvaluateJavaScript(js),
                exception=error.TestError('Failed to evaluate in %s "%s"'
                                          % (url, js)),
                timeout=30)


    def _CheckAdequatelySandboxed(self):
        '''Checks that chrome://sandbox shows "You are adequately sandboxed."'''
        url = 'chrome://sandbox'
        res = self._CheckSandboxPage(url,
                "document.getElementsByTagName('p')[0].textContent")

        text = 'You are adequately sandboxed.'
        if not re.match(text, res):
            raise error.TestFail('Could not find "%s" in %s' % (text, url))


    def _CheckGPUSandboxed(self):
        '''
        Checks that chrome://gpu has "Sandboxed" row, and "Sandboxed" is True.
        '''
        url = 'chrome://gpu'
        res = self._CheckSandboxPage(url,
                                     "browserBridge.isSandboxedForTesting();")
        if res is not True:
            raise error.TestFail('"Sandboxed" not True in %s' % url)

    def run_once(self):
        '''Open various sandbox-related pages and test that we are sandboxed.'''
        with chrome.Chrome() as cr:
            self._tab = cr.browser.tabs[0]
            self._CheckAdequatelySandboxed()
            self._CheckGPUSandboxed()
