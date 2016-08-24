# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import enterprise_policy_base

POLICY_NAME = 'CookiesBlockedForUrls'
URL_BASE = 'http://localhost'
URL_PORT = 8080
URL_HOST = '%s:%d'%(URL_BASE, URL_PORT)
URL_RESOURCE = '/test_data/testWebsite1.html'
TEST_URL = URL_HOST + URL_RESOURCE
COOKIE_NAME = 'cookie1'
COOKIE_BLOCKED_SINGLE_FILE_DATA = [URL_HOST]
COOKIE_BLOCKED_MULTIPLE_FILES_DATA = ['http://google.com', URL_HOST,
                                      'http://doesnotmatter.com']
COOKIE_ALLOWED_MULTIPLE_FILES_DATA = ['https://testingwebsite.html'
                                      'https://somewebsite.com',
                                      'http://doesnotmatter.com']
#Setting the DefaultCookiesSetting to the value 1, allows cookies on all sites.
SUPPORTING_POLICIES = {'DefaultCookiesSetting': 1}


class policy_CookiesBlockedForUrls(enterprise_policy_base.EnterprisePolicyTest):
    """
    Test effect of the CookiesBlockedForUrls policy on Chrome OS behavior.

    This test implicitly verifies one of the settings of the
    DefaultCookiesSetting policy as well. When DefaultCookiesSetting is set to
    1, cookies for all URLs shall be stored (i.e., shall be not blocked), except
    for the URL patterns specified by the CookiesBlockedForUrls policy value.
    The test verifies ChromeOS behaviour for different values of the
    CookiesBlockedForUrls policy eg, for the policy value set to Not Set, Set
    to a single url/host pattern or when the policy is set to multiple url/host
    patterns. It also excercises an additional scenario i.e., it tests that
    cookies are allowed for urls that are not part of the policy value. The
    corresponding three test cases are NotSet_CookiesAllowed,
    SingleUrl_CookiesBlocked, MultipleUrls_CookiesBlocked and
    and MultipleUrls_CookiesAllowed.

    """
    version = 1
    TEST_CASES = {
            'NotSet_CookiesAllowed': '',
            'SingleUrl_CookiesBlocked': COOKIE_BLOCKED_SINGLE_FILE_DATA,
            'MultipleUrls_CookiesBlocked': COOKIE_BLOCKED_MULTIPLE_FILES_DATA,
            'MultipleUrls_CookiesAllowed' : COOKIE_ALLOWED_MULTIPLE_FILES_DATA
            }

    def initialize(self, args=()):
        super(policy_CookiesBlockedForUrls, self).initialize(args)
        self.start_webserver(URL_PORT)

    def _is_cookie_blocked(self, url):
        """
        Returns True if the cookie is blocked for the URL else returns False.

        @param url: Url of the page which is loaded to check whether it's
                    cookie is blocked or stored.

        """
        tab =  self.navigate_to_url(url)
        return tab.GetCookieByName(COOKIE_NAME) is None

    def _test_CookiesBlockedForUrls(self, policy_value, policies_json):
        """
        Verify CrOS enforces CookiesBlockedForUrls policy value.

        When the CookiesBlockedForUrls policy is set to one or more urls/hosts,
        check that cookies are blocked for the urls/urlpatterns listed in
        the policy value. When set to None, check that cookies are allowed for
        all URLs.

        @param policy_value: policy value expected on chrome://policy page.
        @param policies_json: policy JSON data to send to the fake DM server.
        @raises: TestFail if cookies are blocked/not blocked based on the
                 corresponding policy values.

        """
        logging.info('Running _test_CookiesBlockedForUrls(%s, %s)',
                     policy_value, policies_json)
        self.setup_case(POLICY_NAME, policy_value, policies_json)

        cookie_is_blocked = self._is_cookie_blocked(TEST_URL)

        if policy_value and URL_HOST in policy_value:
            if not cookie_is_blocked:
                raise error.TestFail('Cookies should be blocked.')
        else:
            if cookie_is_blocked:
                raise error.TestFail('Cookies should be allowed.')

    def _run_test_case(self, case):
        """
        Setup and run the test configured for the specified test case.

        Set the expected |policy_value| and |policies_json| data based on the
        test |case|. If the user specified an expected |value| in the command
        line args, then use it to set the |policy_value| and blank out the
        |policies_json|.

        @param case: Name of the test case to run.

        """
        policy_value = None
        policies_json = None

        if self.is_value_given:
            # If |value| was given in the command line args, then set expected
            # |policy_value| to the given value, and |policies_json| to None.
            policy_value = self.value
            policies_json = None
        else:
            # Otherwise, set expected |policy_value| and setup |policies_json|
            # data to the values required by the specified test |case|.
            policy_value = ','.join(self.TEST_CASES[case])
            policy_json = {'CookiesBlockedForUrls': self.TEST_CASES[case]}
            policies_json = SUPPORTING_POLICIES.copy()
            policies_json.update(policy_json)

        # Run test using the values configured for the test case.
        self._test_CookiesBlockedForUrls(policy_value, policies_json)

    def run_once(self):
        # The run_once() method is required by autotest. We call the base
        # class run_once_impl() method, which handles command-line run modes,
        # and pass in the standard _run_test_case() method of this test.
            self.run_once_impl(self._run_test_case)
