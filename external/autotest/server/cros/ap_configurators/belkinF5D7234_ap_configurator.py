# Copyright (c) 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

import ap_spec
import belkinF9K_ap_configurator
from selenium.common.exceptions import TimeoutException as \
SeleniumTimeoutException


class BelkinF5D7234APConfigurator(
        belkinF9K_ap_configurator.BelkinF9KAPConfigurator):
    """Class to configure Belkin F5D7234 router."""

    def __init__(self, ap_config):
        super(BelkinF5D7234APConfigurator, self).__init__(ap_config)
        self._dhcp_delay = 0


    def save_page(self, page_number):
        """Save changes and logout from the router.

        @param page_number: the page number to save as an integer.

        """
        xpath = '//input[@type="submit" and @value="Apply Changes"]'
        self.click_button_by_xpath(xpath, alert_handler=self._security_alert)
        self.set_wait_time(30)
        try:
            self.wait.until(lambda _:'setup.htm' in self.driver.title)
        except SeleniumTimeoutException, e:
            try:
                self.wait_for_object_by_xpath(xpath, wait_time=10)
                logging.info("There are no changes to save")
            except:
                dup = '//h1[contains(text(), "Duplicate Administrator")]'
                if (self.driver.find_element_by_xpath(dup)):
                    logging.debug("We got a 'Duplicate Administrator' page "
                                  "when we saved the changes.")
        finally:
            self.restore_default_wait_time()


    def get_supported_modes(self):
        return [{'band': ap_spec.BAND_2GHZ,
                 'modes': [ap_spec.MODE_G, ap_spec.MODE_B,
                           ap_spec.MODE_B | ap_spec.MODE_G]}]


    def set_mode(self, mode):
        self.add_item_to_command_list(self._set_mode, (mode,), 1, 900)


    def _set_mode(self, mode):
        mode_mapping = {ap_spec.MODE_G: '802.11g',
                        ap_spec.MODE_B: '802.11b',
                        ap_spec.MODE_B | ap_spec.MODE_G: '802.11b&802.11g'}
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
                                             wait_for_xpath=key_field,
                                             alert_handler=self._security_alert)
        self.set_content_of_text_field_by_xpath(shared_key, key_field,
                                                abort_check=True)
