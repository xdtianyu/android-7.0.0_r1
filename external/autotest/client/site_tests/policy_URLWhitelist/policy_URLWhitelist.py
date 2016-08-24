# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.client.cros import httpd
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import enterprise_policy_base

POLICY_NAME = 'URLWhitelist'
URL_HOST = 'http://localhost'
URL_PORT = 8080
URL_BASE = '%s:%d/%s' % (URL_HOST, URL_PORT, 'test_data')
BLOCKED_URLS_LIST = [URL_BASE + website for website in
                                    ['/website1.html',
                                     '/website2.html',
                                     '/website3.html']]
SINGLE_WHITELISTED_FILE_DATA = BLOCKED_URLS_LIST[:1]
MULTIPLE_WHITELISTED_FILES_DATA = BLOCKED_URLS_LIST[:2]
BLOCKED_USER_MESSAGE = 'Webpage Blocked'
BLOCKED_ERROR_MESSAGE = 'ERR_BLOCKED_BY_ADMINISTRATOR'
SUPPORTING_POLICIES = {'URLBlacklist': BLOCKED_URLS_LIST}


class policy_URLWhitelist(enterprise_policy_base.EnterprisePolicyTest):
    """
    Test effect of URLWhitleist policy on Chrome OS behavior.

    Navigate to all the websites in the BLOCKED_URLS_LIST. Verify that the
    websites specified by the URLWhitelist policy value are not blocked.
    Also verify that the websites not in the URLWhitelist policy
    value are blocked.

    Two TEST_CASES (SingleWhitelistedFile, MultipleWhitelistedFiles) are
    designed to verify that the functionality works regardless of whether a
    a SINGLE website is specified in the URLWhitelist policy or if MULTIPLE
    websites are specified.
    The third TEST_CASE (NotSet) is designed to verify that all of the websites
    are blocked since the URLWhitelistlist policy is set to None.

    The test case shall pass if the URLs that are part of the URLWhitelist
    policy value are not blocked.
    The test case shall also pass if the URLs that are not part of the
    URLWhitelist policy value are blocked.
    The test case shall fail if the above behavior is not enforced.

    """
    version = 1
    TEST_CASES = {
                 'NotSet': '',
                 'SingleWhitelistedFile': SINGLE_WHITELISTED_FILE_DATA,
                 'MultipleWhitelistedFiles': MULTIPLE_WHITELISTED_FILES_DATA
                 }

    def initialize(self, args=()):
        super(policy_URLWhitelist, self).initialize(args)
        self.start_webserver(URL_PORT)

    def _navigate_to_website(self, url):
        """
        Open a new tab in the browser and navigate to the URL.

        @param url: the website that the browser is navigated to.
        @returns: a chrome browser tab navigated to the URL.

        """
        tab = self.cr.browser.tabs.New()
        logging.info('Navigating to URL:%s', url)
        try:
            tab.Navigate(url, timeout=10)
        except Exception, err:
            logging.error('Timeout Exception in navigating URL: %s\n %s',
                    url, err)
        tab.WaitForDocumentReadyStateToBeComplete()
        return tab

    def _scrape_text_from_website(self, tab):
        """
        Returns a list of the the text content displayed on the page
        matching the page_scrape_cmd filter.

        @param tab: tab containing the website to be parsed.
        @raises: TestFail if the expected text content was not found on the
                 page.

        """
        parsed_message_string = ''
        parsed_message_list = []
        page_scrape_cmd = 'document.getElementById("main-message").innerText;'
        try:
            parsed_message_string = tab.EvaluateJavaScript(page_scrape_cmd)
        except Exception as err:
                raise error.TestFail('Unable to find the expected '
                                     'text content on the test '
                                     'page: %s\n %r'%(tab.url, err))
        logging.info('Parsed message:%s', parsed_message_string)
        parsed_message_list = [str(word) for word in
                               parsed_message_string.split('\n') if word]
        return parsed_message_list

    def _is_url_blocked(self, url):
        """
        Returns True if the URL is blocked else returns False.

        @param url: The URL to be checked whether it is blocked.

        """
        parsed_message_list = []
        tab = self._navigate_to_website(url)
        parsed_message_list = self._scrape_text_from_website(tab)
        if len(parsed_message_list) == 2 and \
                parsed_message_list[0] == 'Website enabled' and \
                parsed_message_list[1] == 'Website is enabled':
            return False

        # Check if the accurate user error message displayed on the error page.
        if parsed_message_list[0] != BLOCKED_USER_MESSAGE or \
                parsed_message_list[1] != BLOCKED_ERROR_MESSAGE:
            logging.warning('The Blocked page user notification '
                            'messages, %s and %s are not displayed on '
                            'the blocked page. The messages may have '
                            'been modified. Please check and update the '
                            'messages in this file accordingly.',
                            BLOCKED_USER_MESSAGE, BLOCKED_ERROR_MESSAGE)
        return True

    def _test_URLWhitelist(self, policy_value, policies_json):
        """
        Verify CrOS enforces URLWhitelist policy value.

        Navigate to all the websites in the BLOCKED_URLS_LIST. Verify that
        the websites specified by the URLWhitelist policy value are not
        blocked. Also verify that the websites not in the URLWhitelist policy
        value are blocked.

        @param policy_value: policy value expected on chrome://policy page.
        @param policies_json: policy JSON data to send to the fake DM server.
        @raises: TestFail if url is blocked/not blocked based on the
                 corresponding policy values.

        """
        url_is_blocked = None
        logging.info('Running _test_Whitelist(%s, %s)',
                     policy_value, policies_json)
        self.setup_case(POLICY_NAME, policy_value, policies_json)

        for url in BLOCKED_URLS_LIST:
            url_is_blocked = self._is_url_blocked(url)
            if policy_value:
                if url in policy_value and url_is_blocked:
                    raise error.TestFail('The URL %s should have been allowed'
                                         ' by policy, but it was blocked' % url)
                elif url not in policy_value and not url_is_blocked:
                    raise error.TestFail('The URL %s should have been blocked'
                                         ' by policy, but it was allowed' % url)

            elif not url_is_blocked:
                raise error.TestFail('The URL %s should have been blocked'
                                      'by policy, but it was allowed' % url)

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
            policies_json = SUPPORTING_POLICIES.copy()
            if not self.TEST_CASES[case]:
                policy_value = None
            else:
                policy_value = ','.join(self.TEST_CASES[case])
                policies_json.update({'URLWhitelist': self.TEST_CASES[case]})

        # Run test using the values configured for the test case.
        self._test_URLWhitelist(policy_value, policies_json)

    def run_once(self):
        self.run_once_impl(self._run_test_case)
