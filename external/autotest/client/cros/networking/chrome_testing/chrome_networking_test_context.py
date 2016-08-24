# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import chrome

class ChromeNetworkingTestContext(object):
    """
    ChromeNetworkingTestContext handles creating a Chrome browser session and
    launching a set of Chrome extensions on it. It provides handles for
    telemetry extension objects, which can be used to inject JavaScript from
    autotest.

    Apart from user provided extensions, ChromeNetworkingTestContext always
    loads the default network testing extension 'network_test_ext' which
    provides some boilerplate around chrome.networkingPrivate calls.

    Example usage:

        context = ChromeNetworkingTestContext()
        context.setup()
        extension = context.network_test_extension()
        extension.EvaluateJavaScript('var foo = 1; return foo + 1;')
        context.teardown()

    ChromeNetworkingTestContext also supports the Python 'with' syntax for
    syntactic sugar.

    """

    NETWORK_TEST_EXTENSION_PATH = ('/usr/local/autotest/cros/networking/'
                                   'chrome_testing/network_test_ext')
    FIND_NETWORKS_TIMEOUT = 5

    # Network type strings used by chrome.networkingPrivate
    CHROME_NETWORK_TYPE_ETHERNET = 'Ethernet'
    CHROME_NETWORK_TYPE_WIFI = 'WiFi'
    CHROME_NETWORK_TYPE_BLUETOOTH = 'Bluetooth'
    CHROME_NETWORK_TYPE_CELLULAR = 'Cellular'
    CHROME_NETWORK_TYPE_VPN = 'VPN'
    CHROME_NETWORK_TYPE_ALL = 'All'

    def __init__(self, extensions=None, username=None, password=None):
        if extensions is None:
            extensions = []
        extensions.append(self.NETWORK_TEST_EXTENSION_PATH)
        self._extension_paths = extensions
        self._username = username
        self._password = password
        self._chrome = None

    def __enter__(self):
        self.setup()
        return self

    def __exit__(self, *args):
        self.teardown()

    def _create_browser(self):
        self._chrome = chrome.Chrome(logged_in=True, gaia_login=True,
                                     extension_paths=self._extension_paths,
                                     username=self._username,
                                     password=self._password)

        # TODO(armansito): This call won't be necessary once crbug.com/251913
        # gets fixed.
        self._ensure_network_test_extension_is_ready()

    def _ensure_network_test_extension_is_ready(self):
        self.network_test_extension.WaitForJavaScriptExpression(
            "typeof chromeTesting != 'undefined'")

    def _get_extension(self, path):
        if self._chrome is None:
            raise error.TestFail('A browser session has not been setup.')
        extension = self._chrome.get_extension(path)
        if extension is None:
            raise error.TestFail('Failed to find loaded extension "%s"' % path)
        return extension

    def setup(self):
        """
        Initializes a ChromeOS browser session that loads the given extensions
        with private API priviliges.

        """
        logging.info('ChromeNetworkingTestContext: setup')
        self._create_browser()
        self.STATUS_PENDING = self.network_test_extension.EvaluateJavaScript(
                'chromeTesting.STATUS_PENDING')
        self.STATUS_SUCCESS = self.network_test_extension.EvaluateJavaScript(
                'chromeTesting.STATUS_SUCCESS')
        self.STATUS_FAILURE = self.network_test_extension.EvaluateJavaScript(
                'chromeTesting.STATUS_FAILURE')

    def teardown(self):
        """
        Closes the browser session.

        """
        logging.info('ChromeNetworkingTestContext: teardown')
        if self._chrome:
            self._chrome.browser.Close()
            self._chrome = None

    @property
    def network_test_extension(self):
        """
        @return Handle to the metworking test Chrome extension instance.
        @raises error.TestFail if the browser has not been set up or if the
                extension cannot get acquired.

        """
        return self._get_extension(self.NETWORK_TEST_EXTENSION_PATH)

    def call_test_function_async(self, function, *args):
        """
        Asynchronously executes a JavaScript function that belongs to
        "chromeTesting.networking" as defined in network_test_ext. The
        return value (or call status) can be obtained at a later time via
        "chromeTesting.networking.callStatus.<|function|>"

        @param function: The name of the function to execute.
        @param args: The list of arguments that are to be passed to |function|.
                Note that strings in JavaScript are quoted using double quotes,
                and this function won't convert string arguments to JavaScript
                strings. To pass a string, the string itself must contain the
                quotes, i.e. '"string"', otherwise the contents of the Python
                string will be compiled as a JS token.
        @raises exceptions.EvaluateException, in case of an error during JS
                execution.

        """
        arguments = ', '.join(str(i) for i in args)
        extension = self.network_test_extension
        extension.ExecuteJavaScript(
            'chromeTesting.networking.' + function + '(' + arguments + ');')

    def wait_for_condition_on_expression_result(
            self, expression, condition, timeout):
        """
        Blocks until |condition| returns True when applied to the result of the
        JavaScript expression |expression|.

        @param expression: JavaScript expression to evaluate.
        @param condition: A function that accepts a single argument and returns
                a boolean.
        @param timeout: The timeout interval length, in seconds, after which
                this method will raise an error.
        @raises error.TestFail, if the conditions is not met within the given
                timeout interval.

        """
        extension = self.network_test_extension
        def _evaluate_expr():
            return extension.EvaluateJavaScript(expression)
        utils.poll_for_condition(
                lambda: condition(_evaluate_expr()),
                error.TestFail(
                        'Timed out waiting for condition on expression: ' +
                        expression),
                timeout)
        return _evaluate_expr()

    def call_test_function(self, timeout, function, *args):
        """
        Executes a JavaScript function that belongs to
        "chromeTesting.networking" and blocks until the function has completed
        its execution. A function is considered to have completed if the result
        of "chromeTesting.networking.callStatus.<|function|>.status" equals
        STATUS_SUCCESS or STATUS_FAILURE.

        @param timeout: The timeout interval, in seconds, for which this
                function will block. If the call status is still STATUS_PENDING
                after the timeout expires, then an error will be raised.
        @param function: The name of the function to execute.
        @param args: The list of arguments that are to be passed to |function|.
                See the docstring for "call_test_function_async" for a more
                detailed description.
        @raises exceptions.EvaluateException, in case of an error during JS
                execution.
        @raises error.TestFail, if the function doesn't finish executing within
                |timeout|.

        """
        self.call_test_function_async(function, *args)
        return self.wait_for_condition_on_expression_result(
                'chromeTesting.networking.callStatus.' + function,
                lambda x: (x is not None and
                           x['status'] != self.STATUS_PENDING),
                timeout)

    def find_cellular_networks(self):
        """
        Queries the current cellular networks.

        @return A list containing the found cellular networks.

        """
        return self.find_networks(self.CHROME_NETWORK_TYPE_CELLULAR)

    def find_wifi_networks(self):
        """
        Queries the current wifi networks.

        @return A list containing the found wifi networks.

        """
        return self.find_networks(self.CHROME_NETWORK_TYPE_WIFI)

    def find_networks(self, network_type):
        """
        Queries the current networks of the queried type.

        @param network_type: One of CHROME_NETWORK_TYPE_* strings.

        @return A list containing the found cellular networks.

        """
        call_status = self.call_test_function(
                self.FIND_NETWORKS_TIMEOUT,
                'findNetworks',
                '"' + network_type + '"')
        if call_status['status'] == self.STATUS_FAILURE:
            raise error.TestFail(
                    'Failed to get networks: ' + call_status['error'])
        networks = call_status['result']
        if type(networks) != list:
            raise error.TestFail(
                    'Expected a list, found "' + repr(networks) + '".')
        return networks
