# Copyright (c) 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


import ap_spec
import belkinF9K_ap_configurator
import logging
from selenium.common.exceptions import TimeoutException as \
SeleniumTimeoutException


class BelkinF6D4230APConfigurator(
        belkinF9K_ap_configurator.BelkinF9KAPConfigurator):
    """Class to configure Belkin F6D4230-4 router."""


    def __init__(self, ap_config):
        super(BelkinF6D4230APConfigurator, self).__init__(ap_config)
        self._dhcp_delay = 0


    def _set_channel(self, channel):
        position = self._get_channel_popup_position(channel)
        channel_choices = ['1', '2', '3', '4', '5', '6', '7', '8',
                           '9', '10', '11']
        xpath = '//select[@name="wchan"]'
        self.select_item_from_popup_by_xpath(channel_choices[position], xpath)


    def _set_mode(self, mode):
        mode_mapping = {ap_spec.MODE_N: '1x1 802.11n',
                        ap_spec.MODE_G: '802.11g',
                        ap_spec.MODE_B | ap_spec.MODE_G | ap_spec.MODE_N:
                        '802.11b&802.11g&802.11n'}
        mode_name = mode_mapping.get(mode)
        if not mode_name:
            raise RuntimeError('The mode %d not supported by router %s. ',
                               hex(mode), self.name)
        xpath = '//select[@name="wbr"]'
        self.select_item_from_popup_by_xpath(mode_name, xpath,
                                             wait_for_xpath=None,
                                             alert_handler=self._security_alert)


    def _set_security_wpapsk(self, security, shared_key, update_interval=None):
        key_field = '//input[@name="wpa_key_pass"]'
        psk = '//select[@name="authentication"]'
        self.select_item_from_popup_by_xpath('WPA/WPA2-Personal (PSK)',
                                             self.security_popup,
                                             wait_for_xpath=key_field,
                                             alert_handler=self._security_alert)
        auth_type = 'WPA-PSK'
        if security == ap_spec.SECURITY_TYPE_WPA2PSK:
            auth_type = 'WPA2-PSK'
        self.select_item_from_popup_by_xpath(auth_type, psk,
                                             alert_handler=self._security_alert)
        self.set_content_of_text_field_by_xpath(shared_key, key_field,
                                                abort_check=True)


    def save_page(self, page_number):
        """Save changes and logout from the router.
        This router has different behaviors while saving the changes everytime.
        Hence I cover all the three possibilities below.

        @param page_number: the page number to save as an integer.

        """
        apply_button = '//input[@type="submit" and @value="Apply Changes"]'
        self.click_button_by_xpath(apply_button,
                                   alert_handler=self._security_alert)
        try:
            self.wait_for_object_by_xpath(apply_button)
        except SeleniumTimeoutException, e:
            try:
                self.set_wait_time(30)
                self.wait.until(lambda _:'setup.htm' in self.driver.current_url)
            except SeleniumTimeoutException, e:
                xpath= '//h1[contains(text(), "Duplicate Administrator")]'
                if (self.driver.find_element_by_xpath(xpath)):
                    logging.debug('We got a \'Duplicate Administrator\' page '
                                  'when we saved the changes.')
            finally:
                self.restore_default_wait_time()
