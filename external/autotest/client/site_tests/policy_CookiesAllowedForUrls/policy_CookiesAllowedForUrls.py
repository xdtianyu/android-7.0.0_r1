# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import enterprise_policy_base

POLICY_NAME = 'CookiesAllowedForUrls'
URL_BASE = 'http://localhost'
URL_PORT = 8080
URL_HOST = '%s:%d'%(URL_BASE, URL_PORT)
URL_RESOURCE = '/test_data/testWebsite1.html'
TEST_URL = URL_HOST + URL_RESOURCE
COOKIE_NAME = 'cookie1'
COOKIE_ALLOWED_SINGLE_FILE_DATA = [URL_HOST]
COOKIE_ALLOWED_MULTIPLE_FILES_DATA = ['http://google.com', URL_HOST,
                                      'http://doesnotmatter.com']
COOKIE_BLOCKED_MULTIPLE_FILES_DATA = ['https://testingwebsite.html',
                                      'https://somewebsite.com',
                                      'http://doesnotmatter.com']
# Setting DefaultCookiesSetting=2 blocks cookies on all sites.
SUPPORTING_POLICIES = {'DefaultCookiesSetting': 2}


class policy_CookiesAllowedForUrls(enterprise_policy_base.EnterprisePolicyTest):
    """Test effect of the CookiesAllowedForUrls policy on Chrome OS behavior.

    This test implicitly verifies one value of the DefaultCookiesSetting
    policy as well. When the DefaultCookiesSetting policy value is set to 2,
    cookies for all URLs shall not be stored (ie, shall be blocked), except
    for the URL patterns specified by the CookiesAllowedForUrls policy.

    The test verifies ChromeOS behaviour for different values of the
    CookiesAllowedForUrls policy i.e., for the policy value set to Not Set,
    Set to a single url/host pattern or when the policy is set to multiple
    url/host patterns. It also excercises an additional scenario i.e., it
    tests that cookies are blocked for urls that are not part of the policy
    value.

    The corresponding three test cases are NotSet_CookiesBlocked,
    SingleUrl_CookiesAllowed, MultipleUrls_CookiesAllowed, and
    and MultipleUrls_CookiesBlocked.

    """
    version = 1
    TEST_CASES = {
        'NotSet_CookiesBlocked': '',
        'SingleUrl_CookiesAllowed': COOKIE_ALLOWED_SINGLE_FILE_DATA,
        'MultipleUrls_CookiesAllowed': COOKIE_ALLOWED_MULTIPLE_FILES_DATA,
        'MultipleUrls_CookiesBlocked': COOKIE_BLOCKED_MULTIPLE_FILES_DATA
    }

    def initialize(self, args=()):
        super(policy_CookiesAllowedForUrls, self).initialize(args)
        self.start_webserver(URL_PORT)

    def _is_cookie_blocked(self, url):
        """Return True if cookie is blocked for the URL else return False.

        @param url: Url of the page which is loaded to check whether it's
                    cookie is blocked or stored.

        """
        tab = self.navigate_to_url(url)
        return tab.GetCookieByName(COOKIE_NAME) is None

    def _test_CookiesAllowedForUrls(self, policy_value, policies_json):
        """Verify CrOS enforces CookiesAllowedForUrls policy value.

        When the CookiesAllowedForUrls policy is set to one or more urls/hosts,
        check that cookies are not blocked for the urls/urlpatterns listed in
        the policy value.
        When set to None, check that cookies are blocked for all URLs.

        @param policy_value: policy value expected on chrome://policy page.
        @param policies_json: policy JSON data to send to the fake DM server.
        @raises: TestFail if cookies are blocked/not blocked based on the
                 corresponding policy values.

        """
        logging.info('Running _test_CookiesAllowedForUrls(%s, %s)',
                     policy_value, policies_json)
        self.setup_case(POLICY_NAME, policy_value, policies_json)

        cookie_is_blocked = self._is_cookie_blocked(TEST_URL)
        logging.info('cookie_is_blocked = %s', cookie_is_blocked)

        if policy_value and URL_HOST in policy_value:
            if cookie_is_blocked:
                raise error.TestFail('Cookies should be allowed.')
        else:
            if not cookie_is_blocked:
                raise error.TestFail('Cookies should be blocked.')

    def run_test_case(self, case):
        """Setup and run the test configured for the specified test case.

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
            policy_json = {POLICY_NAME: self.TEST_CASES[case]}
            policies_json = SUPPORTING_POLICIES.copy()
            policies_json.update(policy_json)

        # Run test using the values configured for the test case.
        self._test_CookiesAllowedForUrls(policy_value, policies_json)
