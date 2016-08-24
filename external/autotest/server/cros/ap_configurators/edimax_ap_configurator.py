# Copyright (c) 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import ap_spec
import dynamic_ap_configurator
import os
import time

from selenium.common.exceptions import ElementNotVisibleException


class EdimaxAPConfigurator(
        dynamic_ap_configurator.DynamicAPConfigurator):
    """Derived class to control the Edimax AP."""

    def navigate_to_page(self, page_number):
        """Navigate to the required page.

        @param page_number: The page number to navigate to.

        """
        if page_number != 1 and page_number != 2:
            raise RuntimeError('Invalid page number passed.  Number of pages is'
                               '%d, page value sent was %d' %
                               (self.get_number_of_pages(), page_number))
        page_url = os.path.join(self.admin_interface_url ,'index.asp')
        self.get_url(page_url, page_title='EDIMAX Technology')
        frame = self.driver.find_element_by_xpath('//frame[@name="mainFrame"]')
        self.driver.switch_to_frame(frame)
        main_tabs = self.driver.find_elements_by_css_selector('div')
        main_tabs[2].click()
        sub_tabs = self.driver.find_elements_by_xpath( \
                                                     '//span[@class="style11"]')
        sub_tabs[2].click()
        if page_number == 1:
            # Open the general settings page.
            self.click_button_by_xpath('//input[@onclick="c_fun(0)" and \
                                        @name="sys"]')
            self.wait_for_object_by_xpath('//select[@name="band"]')
        else:
            # Open the security settings page.
            self.click_button_by_xpath('//input[@onclick="c_fun(1)" and \
                                        @name="sys"]')
            self.wait_for_object_by_xpath('//select[@name="method"]')


    def get_number_of_pages(self):
        return 2


    def save_page(self, page_number):
        """ Save page after applying settings.

        @param page_number: The page number to be saved.

        """
        xpath_ok = ('//input[@name="okbutton"]')
        try:
            self.click_button_by_xpath('//input[@name="B1" and @value="Apply"]',
                                       alert_handler=self._alert_handler)
        except ElementNotVisibleException, e:
            # If the default Apply button is not visible then we are on Security
            # settings page that has a different Apply button. Click that.
            xpath = '//input[@type="submit" and @value="Apply"]'
            apply_tabs = self.driver.find_elements_by_xpath(xpath)
            apply_tabs[2].click()
        self.driver.find_element_by_xpath('//input[@value="APPLY"]').click()
        element = self.wait_for_object_by_xpath(xpath_ok)
        xpath_done = '//input[@name="okbutton" and @value="OK"]'
        while element and not(self.object_by_xpath_exist(xpath_done)):
            time.sleep(0.5)
        self.click_button_by_xpath(xpath_ok, alert_handler=self._alert_handler)


    def get_supported_bands(self):
        return [{'band': ap_spec.BAND_2GHZ, 'channels': range(1, 11)}]


    def set_band(self, band):
        return None


    def get_supported_modes(self):
        return [{'band': ap_spec.BAND_2GHZ,
                 'modes': [ap_spec.MODE_B,
                           ap_spec.MODE_G,
                           ap_spec.MODE_N,
                           ap_spec.MODE_B | ap_spec.MODE_G,
                           ap_spec.MODE_B | ap_spec.MODE_G | ap_spec.MODE_N]}]


    def set_mode(self, mode, band=None):
        self.add_item_to_command_list(self._set_mode, (mode,), 1, 800)


    def _set_mode(self, mode, band=None):
        # Different bands are not supported so we ignore.
        # Create the mode to popup item mapping
        mode_mapping = {ap_spec.MODE_B | ap_spec.MODE_G | ap_spec.MODE_N:
                        '2.4 GHz (B+G+N)',
                        ap_spec.MODE_N: '2.4 GHz (N)',
                        ap_spec.MODE_B: '2.4 GHz (B)',
                        ap_spec.MODE_G: '2.4 GHz (G)',
                        ap_spec.MODE_B | ap_spec.MODE_G: '2.4 GHz (B+G)'}
        mode_name = ''
        if mode in mode_mapping.keys():
            mode_name = mode_mapping[mode]
        else:
            raise RuntimeError('The mode selected %d is not supported by router'
                               ' %s.', hex(mode), self.name)
        xpath = '//select[@name="band"]'
        self.select_item_from_popup_by_xpath(mode_name, xpath)


    def set_channel(self, channel):
        self.add_item_to_command_list(self._set_channel, (channel,), 1, 900)


    def _set_channel(self, channel):
        position = self._get_channel_popup_position(channel)
        channel_choices = ['1', '2', '3', '4', '5',
                           '6', '7', '8', '9', '10', '11']
        self.select_item_from_popup_by_xpath(channel_choices[position],
                                             '//select[@name="chan"]')


    def is_security_mode_supported(self, security_mode):
        """Check if the AP supports this mode of security.

        @param security_mode: Type of security.

        """
        return security_mode in (ap_spec.SECURITY_TYPE_DISABLED,
                                 ap_spec.SECURITY_TYPE_WEP,
                                 ap_spec.SECURITY_TYPE_WPAPSK)


    def set_ssid(self, ssid):
        self.add_item_to_command_list(self._set_ssid, (ssid,), 1, 1000)


    def _set_ssid(self, ssid):
        self.set_content_of_text_field_by_xpath(ssid, '//input[@name="ssid"]',
                                                abort_check=True)
        self._ssid = ssid


    def is_visibility_supported(self):
        """
        Returns if AP supports setting the visibility (SSID broadcast).

        @return True if supported; False otherwise.

        """
        return False


    def is_update_interval_supported(self):
        """
        Returns True if setting the PSK refresh interval is supported.

        @return True is supported; False otherwise.

        """
        return False


    def set_radio(self, enabled=True):
        # AP does not aupport setting Radio.
        return None


    def set_security_disabled(self):
        self.add_item_to_command_list(self._set_security_disabled, (), 2, 1000)


    def _set_security_disabled(self):
        self.select_item_from_popup_by_xpath('Disable',
                                             '//select[@name="method"]')


    def set_security_wpapsk(self, security, shared_key, update_interval=None):
        self.add_item_to_command_list(self._set_security_wpapsk,
                                      (security, shared_key, update_interval),
                                       2, 1000)


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
                                                '//input[@name="pskValue"]')


    def set_security_wep(self, key_value, authentication):
        self.add_item_to_command_list(self._set_security_wep,
                                      (key_value, authentication), 2, 1000)


    def _set_security_wep(self, key_value, authentication):
        self.wait_for_object_by_xpath('//input[@name="key1"]')
        self.select_item_from_popup_by_xpath('WEP', '//select[@name="method"]')
        self.select_item_from_popup_by_xpath('64-bit',
                                             '//select[@name="length"]')
        self.select_item_from_popup_by_xpath('ASCII (5  characters)',
                                             '//select[@name="format"]')
        self.set_content_of_text_field_by_xpath(key_value,
                                                '//input[@name="key1"]')


    def _alert_handler(self, alert):
        """Checks for any modal dialogs which popup to alert the user and
        either raises a RuntimeError or ignores the alert.

        @param alert: The modal dialog's contents.
        """
        text = alert.text
        if 'Pre-Shared Key values should be set at least characters' in text:
            alert.accept()
        else:
            raise RuntimeError('An unexpected modal dialog blocked the'
                               'operation, dialog text: %s', text)
