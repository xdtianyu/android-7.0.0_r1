# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

import common
from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import chromedriver
from autotest_lib.client.cros import httpd


class desktopui_SetFieldsWithChromeDriver(test.test):
    """Test setting a text field via Chrome Driver."""
    version = 1

    def initialize(self):
        """Initialize the test.
        """
        super(desktopui_SetFieldsWithChromeDriver, self).initialize()

        self._test_url = 'http://localhost:8000/hello.html'
        self._expected_title = 'Hello World'
        self._domain = 'localhost'
        self._element_id = '123'
        self._text = 'Hello World'
        self._testServer = httpd.HTTPListener(8000, docroot=self.bindir)
        self._testServer.run()


    def cleanup(self):
        """Clean up the test environment, e.g., stop local http server."""
        if hasattr(self, '_testServer'):
            self._testServer.stop()
        super(desktopui_SetFieldsWithChromeDriver, self).cleanup()


    def run_once(self):
        """Run the test code."""
        with chromedriver.chromedriver() as chromedriver_instance:
            driver = chromedriver_instance.driver
            driver.get(self._test_url)
            logging.info('Expected tab title: %s. Got: %s',
                         self._expected_title, driver.title)
            if driver.title != self._expected_title:
                raise error.TestError('Getting title failed, got title: %s'
                                      % driver.title)

            element = driver.find_element_by_id(self._element_id)
            element.clear()
            element.send_keys(self._text)
            entered_text = element.get_attribute("value")
            if entered_text != self._text:
                raise error.TestError('Value of text box %s, expected %s' %
                                      (self._text, entered_text))

