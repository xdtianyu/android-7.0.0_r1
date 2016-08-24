# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.cros.webstore_test import webstore_test

class webstore_SanityTest(webstore_test):
    """
    Verifies that the CWS landing page works properly.
    """

    version = 1

    def section_header(self, name):
        """
        Returns the XPath of the section header for the given section.

        @param name The name of the section
        """
        return '//div[contains(@class, "wall-structured-section-header")]' + \
                '/div[text() = "%s"]' % name

    sections = ['Featured', 'More recommendations']
    wall_tile = '//div[contains(@class, "webstore-test-wall-tile")]'
    marquee = '//div[contains(@class, "webstore-test-wall-marquee-slideshow")]'

    def run(self):
        self.driver.get(self.webstore_url)

        for section in self.sections:
            self.driver.find_element_by_xpath(self.section_header(section))
        self.driver.find_element_by_xpath(self.wall_tile)
        self.driver.find_element_by_xpath(self.marquee)
