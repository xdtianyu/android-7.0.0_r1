# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import enterprise_policy_base


class policy_RestoreOnStartupURLs(enterprise_policy_base.EnterprisePolicyTest):
    """Test effect of RestoreOnStartupURLs policy on Chrome OS behavior.

    This test verifies the behavior of Chrome OS for a range of valid values
    in the RestoreOnStartupURLs user policy. The values are covered by three
    test cases named: NotSet_NoTabs, SingleUrl_1Tab, and MultipleUrls_3Tabs.
    - Case NotSet_NoTabs opens no tabs. This is the default behavior for
      un-managed user and guest user sessions.
    - Case SingleUrl_1Tab opens a single tab to chrome://settings.
    - Case MultipleUrls_3Tabs opens 3 tabs, in order, to the following pages:
      'chrome://policy', 'chrome://settings', and 'chrome://histograms'

    """
    version = 1

    POLICY_NAME = 'RestoreOnStartupURLs'
    URLS1_DATA = ['chrome://settings']
    URLS3_DATA = ['chrome://policy', 'chrome://settings',
                  'chrome://histograms']
    NEWTAB_URLS = ['chrome://newtab',
                   'https://www.google.com/_/chrome/newtab?espv=2&ie=UTF-8']

    # Dictionary of named test cases and policy data.
    TEST_CASES = {
        'NotSet_NoTabs': None,
        'SingleUrl_1Tab': URLS1_DATA,
        'MultipleUrls_3Tabs': URLS3_DATA
    }

    def _test_startup_urls(self, policy_value, policies_json):
        """Verify CrOS enforces RestoreOnStartupURLs policy value.

        When RestoreOnStartupURLs policy is set to one or more URLs, check
        that a tab is opened to each URL. When set to None, check that no tab
        is opened.

        @param policy_value: policy value expected on chrome://policy page.
        @param policies_json: policy JSON data to send to the fake DM server.

        """
        self.setup_case(self.POLICY_NAME, policy_value, policies_json)
        logging.info('Running _test_StartupURLs(%s, %s)',
                     policy_value, policies_json)

        # Get list of open tab urls from browser; Convert unicode to text;
        # Strip any trailing '/' character reported by devtools.
        tab_urls = [tab.url.encode('utf8').rstrip('/')
                    for tab in reversed(self.cr.browser.tabs)]
        tab_urls_value = ','.join(tab_urls)

        # Telemetry always opens a 'newtab' tab if no startup tabs are opened.
        # If the only open tab is 'newtab', or a tab with the termporary url
        # www.google.com/_/chrome/newtab..., then set tab URLs to None.
        if tab_urls_value in self.NEWTAB_URLS:
            tab_urls_value = None

        # Compare open tabs with expected tabs by |policy_value|.
        if tab_urls_value != policy_value:
            raise error.TestFail('Unexpected tabs: %s '
                                 '(expected: %s)' %
                                 (tab_urls_value, policy_value))

    def _run_test_case(self, case):
        """Setup and run the test configured for the specified test case.

        Set the expected |policy_value| string and |policies_json| data based
        on the test |case|. If the user specified an expected |value| in the
        command line args, then use it to set the |policy_value| and blank out
        the |policies_json|.

        @param case: Name of the test case to run.

        """
        if self.is_value_given:
            # If |value| was given by user, then set expected |policy_value|
            # to the given value, and setup |policies_json| to None.
            policy_value = self.value
            policies_json = None
        else:
            if self.TEST_CASES[case] is None:
                policy_value = None
                policies_json = {'RestoreOnStartup': None}
            else:
                policy_value = ','.join(self.TEST_CASES[case])
                policies_json = {'RestoreOnStartup': 4}
            policy_json = {self.POLICY_NAME: self.TEST_CASES[case]}
            policies_json.update(policy_json)

        # Run test using values configured for the test case.
        self._test_startup_urls(policy_value, policies_json)

    def run_once(self):
        self.run_once_impl(self._run_test_case)
