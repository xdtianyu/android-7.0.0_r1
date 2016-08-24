# Copyright (c) 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


import urlparse

import ap_spec
import belkin_ap_configurator


class BelkinWRTRAPConfigurator(
        belkin_ap_configurator.BelkinAPConfigurator):
    """Base class for Belkin G EB1389 router."""


    def is_security_mode_supported(self, security_mode):
        """
        Returns if a given security_type is supported.

        @param security_mode: one security modes defined in the APSpec

        @return True if the security mode is supported; False otherwise.

        """
        return security_mode in (ap_spec.SECURITY_TYPE_DISABLED,
                                 ap_spec.SECURITY_TYPE_WPAPSK,
                                 ap_spec.SECURITY_TYPE_WPA2PSK,)


    def get_supported_modes(self):
        return [{'band': ap_spec.BAND_2GHZ,
                 'modes': [ap_spec.MODE_G, ap_spec.MODE_G | ap_spec.MODE_B]}]


    def navigate_to_page(self, page_number):
        """
        Navigates to the page corresponding to the given page number.

        This method performs the translation between a page number and a url to
        load. This is used internally by apply_settings.

        @param page_number: page number of the page to load

        """
        if page_number == 1:
            page_url = urlparse.urljoin(self.admin_interface_url,
                                        'wireless_chan.htm')
            self._load_page(page_url, page_title='SSID')
        elif page_number == 2:
            page_url = urlparse.urljoin(self.admin_interface_url,
                                        'wireless_encrypt_64.htm')
            self._load_page(page_url, page_title='Security')
        else:
            raise RuntimeError('Invalid page number passed. Number of pages '
                               '%d, page value sent was %d' %
                               (self.get_number_of_pages(), page_number))


    def _load_page(self, page_url, page_title):
        """
        Load the given page and login if required.

        @param page_url: The complete page url to load.
        @param page_title: The page title that we expect.
        """
        try:
            self.get_url(page_url, page_title)
        except:
            if 'Login' in self.driver.title:
                xpath = '//input[@name="login_password"]'
                self.set_content_of_text_field_by_xpath('password', xpath,
                                                        abort_check=True)
                submit = '//input[@class="submitBtn" and @value="Submit"]'
                self.click_button_by_xpath(submit)
        finally:
            self.wait.until(lambda _: page_title in self.driver.title)


    def save_page(self, page_number):
        """
        Saves the given page.

        @param page_number: Page number of the page to save.

        """
        xpath = '//input[@value="Apply Changes" and @class="submitBtn"]'
        self.click_button_by_xpath(xpath, alert_handler=self._security_alert)
        # Give belkin some time to save settings and come to login page.
        try:
            self.wait_for_object_by_xpath('//input[@name="login_password"]',
                                          wait_time=20)
        except:
            raise RuntimeError('Settings were not saved. We did not find the '
                               'login text field.')


    def set_mode(self, mode):
        self.add_item_to_command_list(self._set_mode, (mode,), 1, 900)


    def _set_mode(self, mode):
        mode_mapping = {ap_spec.MODE_G | ap_spec.MODE_B: '802.11g&802.11b',
                        ap_spec.MODE_G: '802.11g Only'}
        mode_name = mode_mapping.get(mode)
        if not mode_name:
            raise RuntimeError('The mode %d not supported by router %s. ' %
                               hex(mode), self.name)
        xpath = '//select[@name="wl_gmode"]'
        self.select_item_from_popup_by_xpath(mode_name, xpath)


    def set_security_wep(self, key_value, authentication):
        self.add_item_to_command_list(self._set_security_wep,
                                      (key_value, authentication), 2, 1000)


    def _set_security_wep(self, key_value, authentication):
        # WEP has been removed from the list of supported security options
        # since we get a Duplicate Administrator page everytime
        # we generate a key for WEP.
        popup = '//select[@name="wl_sec_mode"]'
        self.wait_for_object_by_xpath(popup)
        text_field = '//input[@name="wep64pp"]'
        self.select_item_from_popup_by_xpath('64bit WEP', popup,
                                             wait_for_xpath=text_field)
        self.set_content_of_text_field_by_xpath(key_value, text_field,
                                                abort_check=True)
        generate = '//input[@name="wep128a_btn" and @value="generate"]'
        self.click_button_by_xpath(generate)
        self.wait_for_object_by_xpath(text_field)


    def set_security_wpapsk(self, security, shared_key, update_interval=None):
        self.add_item_to_command_list(self._set_security_wpapsk,
                                      (security, shared_key, update_interval),
                                      2, 900)


    def _set_security_wpapsk(self, security, shared_key, update_interval=None):
        popup = '//select[@name="wl_sec_mode"]'
        self.wait_for_object_by_xpath(popup)
        key_field = '//input[@name="wl_wpa_psk1"]'
        self.select_item_from_popup_by_xpath('WPA-PSK(no server)', popup,
                                             wait_for_xpath=key_field)
        self.set_content_of_text_field_by_xpath(shared_key, key_field,
                                                abort_check=False)
        security_popup = 'WPA-PSK'
        if security == ap_spec.SECURITY_TYPE_WPA2PSK:
            security_popup = 'WPA2-PSK'
        self.select_item_from_popup_by_xpath(security_popup,
                                             '//select[@name="wl_auth"]')

