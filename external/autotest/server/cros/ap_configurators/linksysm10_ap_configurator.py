# Copyright (c) 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from selenium.common.exceptions import WebDriverException

import linksyse1500_ap_configurator


class LinksysM10APConfigurator(
        linksyse1500_ap_configurator.Linksyse1500APConfigurator):
    """Base class for objects to configure Linksys m10 and m20."""


    def save_page(self, page_number):
        """Saves the page.

        @param page_number: the page number as an integer

        """
        submit_btn = '//a[@href="javascript:to_submit(document.forms[0])"]'
        continue_btn = '//input[@value="Continue" and @onclick="to_submit()"]'
        try:
            self.click_button_by_xpath(submit_btn,
                                       alert_handler=self._sec_alert)
            self.wait_for_object_by_xpath(continue_btn, wait_time=10)
            self.click_button_by_xpath(continue_btn,
                                       alert_handler=self._sec_alert)
        except WebDriverException, e:
            self._check_for_alert_in_message(str(e),
                                             alert_handler=self._sec_alert)
