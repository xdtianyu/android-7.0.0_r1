# Copyright (c) 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import belkinF9K_ap_configurator
import logging
from selenium.common.exceptions import TimeoutException as \
SeleniumTimeoutException


class BelkinF7D1301APConfigurator(
        belkinF9K_ap_configurator.BelkinF9KAPConfigurator):
    """Class to configure Belkin F7D1301 v1 (01) router."""

    def __init__(self, ap_config):
        super(BelkinF7D1301APConfigurator, self).__init__(ap_config)
        self._dhcp_delay = 0


    def save_page(self, page_number):
        """
        Save changes and logout from the router.

        @param page_number: the page number to save as an integer.

        """
        button = '//input[@type="submit" and @value="Apply Changes"]'
        self.click_button_by_xpath(button,
                                   alert_handler=self._security_alert)
        self.set_wait_time(30)
        try:
            self.wait.until(lambda _:'index.htm' in self.driver.title)
        except SeleniumTimeoutException, e:
            xpath= '//h1[contains(text(), "Duplicate Administrator")]'
            if (self.driver.find_element_by_xpath(xpath)):
                logging.debug('We got a \'Duplicate Administrator\' page '
                              'when we saved the changes.')
            else:
                raise RuntimeError("We couldn't save the page" + str(e))
        finally:
            self.restore_default_wait_time()
