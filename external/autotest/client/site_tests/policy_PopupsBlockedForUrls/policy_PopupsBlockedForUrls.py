# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import utils

from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import enterprise_policy_base


class policy_PopupsBlockedForUrls(enterprise_policy_base.EnterprisePolicyTest):
    """Test PopupsBlockedForUrls policy effect on CrOS look & feel.

    This test verifies the behavior of Chrome OS with a range of valid values
    for the PopupsBlockedForUrls user policy, when DefaultPopupsSetting=1
    (i.e., allow popups by default on all pages except those in domains listed
    in PopupsBlockedForUrls). These valid values are covered by 4 test cases,
    named: NotSet_Allowed, 1Url_Blocked, 2Urls_Allowed, and 3Urls_Blocked.

    When the policy value is None (as in case NotSet_Allowed), then popups are
    allowed on any page. When the value is set to one or more URLs (as in
    1Url_Blocked, 2Urls_Allowed, and 3Urls_Blocked), popups are blocked only
    on pages with a domain that matches any of the listed URLs, and allowed on
    any of those that do not match.

    As noted above, this test requires the DefaultPopupsSetting policy to be
    set to 1. A related test, policy_PopupsAllowedForUrls, requires the value
    to be set to 2. That value blocks popups on all pages except those with
    domains listed in PopupsAllowedForUrls.

    """
    version = 1

    POLICY_NAME = 'PopupsBlockedForUrls'
    URL_HOST = 'http://localhost'
    URL_PORT = 8080
    URL_BASE = '%s:%d' % (URL_HOST, URL_PORT)
    URL_PAGE = '/popup_status.html'
    TEST_URL = URL_BASE + URL_PAGE

    URL1_DATA = [URL_HOST]
    URL2_DATA = ['http://www.bing.com', 'https://www.yahoo.com']
    URL3_DATA = ['http://www.bing.com', URL_BASE,
                 'https://www.yahoo.com']
    TEST_CASES = {
        'NotSet_Allowed': '',
        '1Url_Blocked': URL1_DATA,
        '2Urls_Allowed': URL2_DATA,
        '3Urls_Blocked': URL3_DATA
    }
    STARTUP_URLS = ['chrome://policy', 'chrome://settings']
    SUPPORTING_POLICIES = {
        'DefaultPopupsSetting': 1,
        'BookmarkBarEnabled': False,
        'RestoreOnStartupURLs': STARTUP_URLS,
        'RestoreOnStartup': 4
    }

    def initialize(self, args=()):
        super(policy_PopupsBlockedForUrls, self).initialize(args)
        self.start_webserver(self.URL_PORT)

    def cleanup(self):
        if self._web_server:
            self._web_server.stop()
        super(policy_PopupsBlockedForUrls, self).cleanup()

    def _wait_for_page_ready(self, tab):
        utils.poll_for_condition(
            lambda: tab.EvaluateJavaScript('pageReady'),
            exception=error.TestError('Test page is not ready.'))

    def _test_popups_blocked_for_urls(self, policy_value, policies_json):
        """Verify CrOS enforces the PopupsBlockedForUrls policy.

        When PopupsBlockedForUrls is undefined, popups shall be allowed on
        all pages. When PopupsBlockedForUrls contains one or more URLs, popups
        shall be blocked only on the pages whose domain matches any of the
        listed URLs.

        @param policy_value: policy value expected on chrome://policy page.
        @param policies_json: policy JSON data to send to the fake DM server.

        """
        self.setup_case(self.POLICY_NAME, policy_value, policies_json)
        logging.info('Running _test_popups_blocked_for_urls(%s, %s)',
                     policy_value, policies_json)

        tab = self.navigate_to_url(self.TEST_URL)
        self._wait_for_page_ready(tab)
        is_blocked = tab.EvaluateJavaScript('isPopupBlocked();')

        # String |URL_HOST| will be found in string |policy_value| for
        # test cases 1Url_Blocked and 3Urls_Blocked, but not for cases
        # NotSet_Allowed and 2Urls_Allowed.
        if policy_value is not None and self.URL_HOST in policy_value:
            if not is_blocked:
                raise error.TestFail('Popups should be blocked.')
        else:
            if is_blocked:
                raise error.TestFail('Popups should not be blocked.')
        tab.Close()

    def _run_test_case(self, case):
        """Setup and run the test configured for the specified test case.

        Set the expected |policy_value| and |policies_json| data based on the
        test |case|. If the user specified an expected |value| in the command
        line args, then use it to set the |policy_value| and blank out the
        |policies_json|.

        @param case: Name of the test case to run.

        """
        if self.is_value_given:
            # If |value| was given in args, then set expected |policy_value|
            # to the given value, and setup |policies_json| data to None.
            policy_value = self.value
            policies_json = None
        else:
            # Otherwise, set expected |policy_value| and setup |policies_json|
            # data to the values specified by the test |case|.
            policy_value = ','.join(self.TEST_CASES[case])
            policy_json = {self.POLICY_NAME: self.TEST_CASES[case]}
            policies_json = self.SUPPORTING_POLICIES.copy()
            policies_json.update(policy_json)

        # Run test using the values configured for the test case.
        self._test_popups_blocked_for_urls(policy_value, policies_json)

    def run_once(self):
        # The run_once() method is required by autotest. We call the base
        # class run_once_impl() method, which handles command-line run modes,
        # and pass in the standard _run_test_case() method of this test.
        self.run_once_impl(self._run_test_case)
