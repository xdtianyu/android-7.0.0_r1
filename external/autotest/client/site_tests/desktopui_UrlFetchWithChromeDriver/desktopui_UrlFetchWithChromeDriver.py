# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

import common
from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import chromedriver
from autotest_lib.client.cros import httpd


class desktopui_UrlFetchWithChromeDriver(test.test):
    """Test fetching url and cookie using Chrome Driver."""
    version = 1

    def initialize(self, live=True):
        """Initialize the test.

        @param live: Set to True to access external websites. Otherwise, test
                     with localhost http server. Default value is set to True.
        """
        self._live = live
        super(desktopui_UrlFetchWithChromeDriver, self).initialize()

        if self._live:
            self._test_url = 'http://www.msn.com/'
            self._expected_title = 'MSN.com'
            self._domain = '.msn.com'
        else:
            self._test_url = 'http://localhost:8000/hello.html'
            self._expected_title = 'Hello World'
            self._domain = 'localhost'
            self._testServer = httpd.HTTPListener(8000, docroot=self.bindir)
            self._testServer.run()


    def cleanup(self):
        """Clean up the test environment, e.g., stop local http server."""
        if not self._live and hasattr(self, '_testServer'):
            self._testServer.stop()
        super(desktopui_UrlFetchWithChromeDriver, self).cleanup()


    def run_once(self):
        """Run the test code."""
        with chromedriver.chromedriver() as chromedriver_instance:
            driver = chromedriver_instance.driver
            driver.delete_all_cookies()
            driver.get(self._test_url)

            logging.info('Expected tab title: %s. Got: %s',
                         self._expected_title, driver.title)
            if driver.title != self._expected_title:
                raise error.TestError('Getting title failed, got title: %s'
                                      % driver.title)

            cookie_found = any([cookie for cookie in
                                driver.get_cookies()
                                if cookie['domain'] == self._domain])
            if not cookie_found:
                raise error.TestError('Expected cookie for %s' % self._test_url)

