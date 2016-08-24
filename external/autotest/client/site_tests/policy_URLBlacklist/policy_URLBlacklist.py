# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.client.cros import httpd
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import enterprise_policy_base

POLICY_NAME = 'URLBlacklist'
URL_HOST = 'http://localhost'
URL_PORT = 8080
URL_BASE = '%s:%d/%s' % (URL_HOST, URL_PORT, 'test_data')
ALL_URLS_LIST = [URL_BASE + website for website in
                  ['/website1.html',
                   '/website2.html',
                   '/website3.html']]
SINGLE_BLACKLISTED_FILE_DATA = ALL_URLS_LIST[:1]
MULTIPLE_BLACKLISTED_FILES_DATA = ALL_URLS_LIST[:2]
BLOCKED_USER_MESSAGE = 'Webpage Blocked'
BLOCKED_ERROR_MESSAGE = 'ERR_BLOCKED_BY_ADMINISTRATOR'


class policy_URLBlacklist(enterprise_policy_base.EnterprisePolicyTest):
    """
    Test effect of URLBlacklist policy on Chrome OS behavior.

    Navigate to each the URLs in the ALL_URLS_LIST and verify that the URLs
    specified by the URLBlackList policy are blocked.
    Throw a warning if the user message on the blocked page is incorrect.

    Two test cases (SingleBlacklistedFile, MultipleBlacklistedFiles) are
    designed to verify that URLs specified in the URLBlacklist policy are
    blocked.
    The third test case(NotSet) is designed to verify that none of the URLs
    are blocked since the URLBlacklist policy is set to None

    The test case shall pass if the URLs that are part of the URLBlacklist
    policy value are blocked.
    The test case shall also pass if the URLs that are not part of the
    URLBlacklist policy value are not blocked.
    The test case shall fail if the above behavior is not enforced.

    """
    version = 1
    TEST_CASES = {
        'NotSet': '',
        'SingleBlacklistedFile': SINGLE_BLACKLISTED_FILE_DATA,
        'MultipleBlacklistedFiles': MULTIPLE_BLACKLISTED_FILES_DATA,
    }

    def initialize(self, args=()):
        super(policy_URLBlacklist, self).initialize(args)
        self.start_webserver(URL_PORT)

    def navigate_to_website(self, url):
        """
        Open a new tab in the browser and navigate to the URL.

        @param url: the URL that the browser is navigated to.
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

    def scrape_text_from_website(self, tab):
        """
        Returns a list of the text content matching the page_scrape_cmd filter.

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
                        'text content on the test page: %s\n %r'%(tab.url, err))
        logging.info('Parsed message:%s', parsed_message_string)
        parsed_message_list = [str(word) for word in
                               parsed_message_string.split('\n') if word]
        return parsed_message_list

    def is_url_blocked(self, url):
        """
        Returns True if the URL is blocked else returns False.

        @param url: The URL to be checked whether it is blocked.

        """
        parsed_message_list = []
        tab = self.navigate_to_website(url)
        parsed_message_list = self.scrape_text_from_website(tab)
        if len(parsed_message_list) == 2 and \
                parsed_message_list[0] == 'Website enabled' and \
                parsed_message_list[1] == 'Website is enabled':
            return False

        #Check if the accurate user error message displayed on the error page.
        if parsed_message_list[0] != BLOCKED_USER_MESSAGE or \
                parsed_message_list[1] != BLOCKED_ERROR_MESSAGE:
            logging.warning('The Blocked page user notification '
                            'messages, %s and %s are not displayed on '
                            'the blocked page. The messages may have '
                            'been modified. Please check and update the '
                            'messages in this file accordingly.',
                            BLOCKED_USER_MESSAGE, BLOCKED_ERROR_MESSAGE)
        return True

    def _test_URLBlacklist(self, policy_value, policies_json):
        """
        Verify CrOS enforces URLBlacklist policy value.

        When the URLBlacklist policy is set to one or more Domains,
        check that navigation to URLs in the Blocked list are blocked.
        When set to None, check that none of the websites are blocked.

        @param policy_value: policy value expected on chrome://policy page.
        @param policies_json: policy JSON data to send to the fake DM server.
        @raises: TestFail if url is blocked/not blocked based on the
                 corresponding policy values.

        """
        url_is_blocked = None
        self.setup_case(POLICY_NAME, policy_value, policies_json)
        logging.info('Running _test_URLBlacklist(%s, %s)',
                     policy_value, policies_json)

        for url in ALL_URLS_LIST:
            url_is_blocked = self.is_url_blocked(url)
            if policy_value:
                if url in policy_value and not url_is_blocked:
                    raise error.TestFail('The URL %s should have been blocked'
                                         ' by policy, but it was allowed' % url)
            elif url_is_blocked:
                raise error.TestFail('The URL %s should have been allowed'
                                      'by policy, but it was blocked' % url)

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

        if case not in self.TEST_CASES:
            raise error.TestError('Test case %s is not valid.' % case)
        logging.info('Running test case: %s', case)

        if self.is_value_given:
            # If |value| was given in the command line args, then set expected
            # |policy_value| to the given value, and |policies_json| to None.
            policy_value = self.value
            policies_json = None
        else:
            # Otherwise, set expected |policy_value| and setup |policies_json|
            # data to the values required by the test |case|.
            if case == 'NotSet':
                policy_value = None
                policies_json = {'URLBlacklist': None}
            elif case == 'SingleBlacklistedFile':
                policy_value = ','.join(SINGLE_BLACKLISTED_FILE_DATA)
                policies_json = {'URLBlacklist': SINGLE_BLACKLISTED_FILE_DATA}

            elif case == 'MultipleBlacklistedFiles':
                policy_value = ','.join(MULTIPLE_BLACKLISTED_FILES_DATA)
                policies_json = {
                        'URLBlacklist': MULTIPLE_BLACKLISTED_FILES_DATA
                        }

        # Run test using the values configured for the test case.
        self._test_URLBlacklist(policy_value, policies_json)

    def run_once(self):
            self.run_once_impl(self._run_test_case)
