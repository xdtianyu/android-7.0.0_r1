# Copyright (c) 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import ap_spec
import edimax_ap_configurator
import os
import time

class Edimax6428nsAPConfigurator(
        edimax_ap_configurator.EdimaxAPConfigurator):
    """Derived class to control the Edimax-Br6428 AP."""

    def navigate_to_page(self, page_number):
        """Navigate to the required page.

        @param page_number: The page number to navigate to.

        """
        if page_number == 1:
            # Open the general settings page.
            page_url = os.path.join(self.admin_interface_url ,'wlbasic.asp')
            self.get_url(page_url)
            self.wait_for_object_by_xpath('//select[@name="band"]')
        elif page_number == 2:
            # Open the security settings page.
            page_url = os.path.join(self.admin_interface_url ,'wlencrypt.asp')
            self.get_url(page_url)
            self.wait_for_object_by_xpath('//select[@name="method"]')
        else:
            raise RuntimeError('Invalid page number passed.  Number of pages is'
                               '%d, page value sent was %d' %
                               (self.get_number_of_pages(), page_number))


    def save_page(self, page_number):
        """ Save page after applying settings.

        @param page_number: The page number to be saved.

        """
        xpath_ok = ('//input[@name="okbutton"]')
        self.click_button_by_xpath('//input[@src="graphics/apply1.gif"]',
                                    alert_handler=self._alert_handler)
        self.driver.find_element_by_xpath('//input[@value="APPLY"]').click()
        element = self.wait_for_object_by_xpath(xpath_ok)
        xpath_done = '//input[@name="okbutton" and @value="OK"]'
        while element and not(self.object_by_xpath_exist(xpath_done)):
            time.sleep(0.5)
        self.click_button_by_xpath(xpath_ok, alert_handler=self._alert_handler)


    def _set_security_wpapsk(self, security, shared_key, update_interval=None):
        self.wait_for_object_by_xpath('//input[@name="pskValue"]')
        self.select_item_from_popup_by_xpath('WPA pre-shared key',
                                             '//select[@name="method"]')
        if security == ap_spec.SECURITY_TYPE_WPAPSK:
            wpa_item = 'wpaCipher1'
        else:
            wpa_item = 'wpaCipher2'
        self.click_button_by_id(wpa_item, alert_handler=self._alert_handler)
        self.select_item_from_popup_by_xpath('Passphrase',
                                             '//select[@name="pskFormat"]')
        self.set_content_of_text_field_by_xpath(shared_key,
                                                '//input[@name="pskValue"]',
                                                abort_check=True)


    def _set_security_wep(self, key_value, authentication):
        self.wait_for_object_by_xpath('//input[@name="key1"]')
        self.select_item_from_popup_by_xpath('WEP', '//select[@name="method"]')
        self.select_item_from_popup_by_xpath('64-bit',
                                             '//select[@name="length"]')
        self.select_item_from_popup_by_xpath('ASCII (5  characters)',
                                             '//select[@name="format"]')
        self.set_content_of_text_field_by_xpath(key_value,
                                                '//input[@name="key1"]',
                                                abort_check=True)
