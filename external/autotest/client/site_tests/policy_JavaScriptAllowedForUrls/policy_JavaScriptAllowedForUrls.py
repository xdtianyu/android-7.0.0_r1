# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import time
import utils

from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import enterprise_policy_base


class policy_JavaScriptAllowedForUrls(
    enterprise_policy_base.EnterprisePolicyTest):
    """Test JavaScriptAllowedForUrls policy effect on CrOS look & feel.

    This test verifies the behavior of Chrome OS with a range of valid values
    for the JavaScriptAllowedForUrls user policies. These values are covered
    by four test cases, named: NotSet_BlockJS, SingleUrl_AllowJS,
    MultipleUrls_BlockJS, and MultipleUrls_AllowJS.

    When the policy value is None (as in case=NotSet_BlockJS), then
    JavaScript will be blocked on any page. When the value is set to a single
    URL pattern (as in case=SingleUrl_AllowJS), JavaScript will be allowed on
    any page that matches that pattern. When set to multiple URL patterns (as
    in case=MultipleUrls_BlockJS or MultipleUrls_AllowJS) then JavaScript will
    be allowed on any page with a URL that matches any of the listed patterns.

    Two test cases (SingleUrl_AllowJS, MultipleUrls_AllowJS) are designed to
    allow JavaScript to run on the test page. The other two test cases
    (NotSet_BlockJS, MultipleUrls_BlockJS) are designed to block JavaScript
    from running on the test page.

    Note this test has a dependency on the DefaultJavaScriptSetting policy,
    which is partially tested herein, and in policy_JavaScriptBlockedForUrls.
    For this test, we set DefaultJavaScriptSetting=2. This blocks JavaScript
    on all pages except those with a URL matching a pattern in
    JavaScriptAllowedForUrls. For the test policy_JavaScriptBlockedForUrls, we
    set DefaultJavaScriptSetting=1. That allows JavaScript to be run on all
    pages except those with URLs that match patterns listed in
    JavaScriptBlockedForUrls.

    """
    version = 1

    POLICY_NAME = 'JavaScriptAllowedForUrls'
    URL_HOST = 'http://localhost'
    URL_PORT = 8080
    URL_BASE = '%s:%d' % (URL_HOST, URL_PORT)
    URL_PAGE = '/js_test.html'
    TEST_URL = URL_BASE + URL_PAGE

    TEST_CASES = {
        'NotSet_BlockJS': None,
        'SingleUrl_AllowJS': [URL_BASE],
        'MultipleUrls_BlockJS': ['http://www.bing.com',
                                 'https://www.yahoo.com'],
        'MultipleUrls_AllowJS': ['http://www.bing.com',
                                 TEST_URL,
                                 'https://www.yahoo.com']
    }

    STARTUP_URLS = ['chrome://policy', 'chrome://settings']
    SUPPORTING_POLICIES = {
        'DefaultJavaScriptSetting': 2,
        'BookmarkBarEnabled': False,
        'RestoreOnStartupURLs': STARTUP_URLS,
        'RestoreOnStartup': 4
    }

    def initialize(self, args=()):
        super(policy_JavaScriptAllowedForUrls, self).initialize(args)
        self.start_webserver(self.URL_PORT)

    def _can_execute_javascript(self, tab):
        """Determine whether JavaScript is allowed to run on the given page.

        @param tab: browser tab containing JavaScript to run.

        """
        try:
            utils.poll_for_condition(
                lambda: tab.EvaluateJavaScript('jsAllowed', timeout=2),
                exception=error.TestError('Test page is not ready.'))
            return True
        except:
            return False

    def _test_javascript_allowed_for_urls(self, policy_value, policies_json):
        """Verify CrOS enforces the JavaScriptAllowedForUrls policy.

        When JavaScriptAllowedForUrls is undefined, JavaScript shall be blocked
        on all pages. When JavaScriptAllowedForUrls contains one or more URL,
        patterns, JavaScript shall be allowed to run only on the pages whose
        URL matches any of the listed patterns.

        @param policy_value: policy value expected on chrome://policy page.
        @param policies_json: policy JSON data to send to the fake DM server.

        """
        self.setup_case(self.POLICY_NAME, policy_value, policies_json)
        logging.info('Running _test_javascript_allowed_for_urls(%s, %s)',
                     policy_value, policies_json)

        tab = self.cr.browser.tabs.New()
        tab.Activate()
        tab.Navigate(self.TEST_URL)
        time.sleep(1)

        utils.poll_for_condition(
            lambda: (tab.url == self.TEST_URL),
            exception=error.TestError('Test page is not ready.'))
        javascript_is_allowed = self._can_execute_javascript(tab)

        if policy_value is not None and self.URL_HOST in policy_value:
            # If |URL_HOST| is in |policy_value|, then JavaScript execution
            # should be allowed. If execution is blocked, raise an error.
            if not javascript_is_allowed:
                raise error.TestFail('JavaScript should be allowed.')
        else:
            if javascript_is_allowed:
                raise error.TestFail('JavaScript should be blocked.')
        tab.Close()

    def _run_test_case(self, case):
        """Setup and run the test configured for the specified test case.

        Set the expected |policy_value| string and |policies_json| data based
        on the test |case|. If the user specified an expected |value| in the
        command line args, then use it to set the |policy_value| and blank out
        the |policies_json|.

        @param case: Name of the test case to run.

        """
        if self.is_value_given:
            # If |value| was given in the command line args, then set expected
            # |policy_value| to the given value, and |policies_json| to None.
            policy_value = self.value
            policies_json = None
        else:
            # Otherwise, set expected |policy_value| and setup |policies_json|
            # data to the values required by the specified test |case|.
            if not self.TEST_CASES[case]:
                policy_value = None
            else:
                policy_value = ','.join(self.TEST_CASES[case])
            policy_json = {'JavaScriptAllowedForUrls': self.TEST_CASES[case]}
            policies_json = self.SUPPORTING_POLICIES.copy()
            policies_json.update(policy_json)

        # Run test using the values configured for the test |case|.
        self._test_javascript_allowed_for_urls(policy_value, policies_json)

    def run_once(self):
        self.run_once_impl(self._run_test_case)
