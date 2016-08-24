# Copyright (c) 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os

import ap_spec
import trendnet_ap_configurator


class Trendnet654trAPConfigurator(trendnet_ap_configurator.
                                  TrendnetAPConfigurator):
    """Derived class to control the Trendnet TEW-654TR."""


    def _alert_handler(self, alert):
        """
        Checks for any modal dialogs which popup to alert the user and
        either raises a RuntimeError or ignores the alert.

        @param alert: The modal dialog's contents.
        """
        text = alert.text
        if 'The length of Key1 must be 5 characters' in text:
            alert.accept()
        else:
            raise RuntimeError('An unexpected alert was thrown: %s' % text)


    def get_number_of_pages(self):
        return 1


    def navigate_to_page(self, page_number):
        """
        Navigates to the page corresponding to the given page number.

        This method performs the translation between a page number and a url to
        load. This is used internally by apply_settings.

        @param page_number: page number of the page to load

        """
        self.navigate_to_login_page()
        if page_number == 1:
            page_url = os.path.join(self.admin_interface_url,'wireless.htm')
            self.get_url(page_url, page_title='TEW-654TR')
        else:
            raise RuntimeError('Invalid page number passed.  Number of pages '
                               '%d, page value sent was %d' %
                               (self.get_number_of_pages(), page_number))


    def navigate_to_login_page(self):
        """Navigates through the login page.

        If we are logged out during and time this method walks through the login
        process so the appropriate page is loaded to update the settings.

        """
        # We need to login first in order to configure settings.
        self.get_url(self.admin_interface_url, page_title='TEW-654TR')
        self.wait_for_object_by_id('user_name')
        self.set_content_of_text_field_by_id('admin', 'user_name',
                                             abort_check=True)
        self.set_content_of_text_field_by_id('password', 'user_pwd',
                                             abort_check=True)
        self.click_button_by_id('login')


    def save_page(self, page_number):
        """
        Saves the given page.

        @param page_number: Page number of the page to save.

        """
        xpath = ('//a[contains(@href,"send_request")]//img')
        self.click_button_by_xpath(xpath, alert_handler=self._alert_handler)
        self.wait_for_object_by_id('back_btn')
        self.click_button_by_id('back_btn')


    def set_radio(self, enabled=True):
        return None


    def _set_ssid(self, ssid):
        self.set_content_of_text_field_by_id(ssid, 'ssid')
        self._ssid = ssid


    def _set_channel(self, channel):
        position = self._get_channel_popup_position(channel)
        channel_choices = ['1', '2', '3', '4', '5', '6', '7'
                           '8', '9', '10', '11']
        self.select_item_from_popup_by_id(channel_choices[position], 'channel')


    def set_band(self, band):
        return None


    def _set_mode(self, mode, band=None):
        # Different bands are not supported so we ignore.
        # Create the mode to popup item mapping
        mode_mapping = {ap_spec.MODE_B| ap_spec.MODE_G | ap_spec.MODE_N:
                        '2.4Ghz 802.11b/g/n mixed mode',
                        ap_spec.MODE_N: '2.4Ghz 802.11n only mode',
                        ap_spec.MODE_B | ap_spec.MODE_G:
                        '2.4Ghz 802.11b/g mixed mode'}
        mode_name = ''
        if mode in mode_mapping.keys():
            mode_name = mode_mapping[mode]
        else:
            raise RuntimeError('The mode selected %s is not supported by router'
                               ' %s.', ap_spec.mode_string_for_mode(mode),
                               self.name)
        self.select_item_from_popup_by_id(mode_name, 'dot11_mode')


    def set_security_disabled(self):
        self.add_item_to_command_list(self._set_security_disabled, (), 1, 100)


    def _set_security_disabled(self):
        self.wait_for_object_by_id('security_type')
        item = 'Disable Wireless Security ( not recommended )'
        self.select_item_from_popup_by_id(item, 'security_type')


    def set_security_wep(self, key_value, authentication):
        self.add_item_to_command_list(self._set_security_wep,
                                      (key_value, authentication), 1, 100)


    def _set_security_wep(self, key_value, authentication):
        self.wait_for_object_by_id('security_type')
        item = 'Enable WEP Wireless Security ( basic )'
        self.select_item_from_popup_by_id(item, 'security_type')
        self.set_content_of_text_field_by_id(key_value, 'wep_key1',
                                             abort_check=True)


    def set_security_wpapsk(self, security, shared_key, update_interval=1800):
        self.add_item_to_command_list(self._set_security_wpapsk,
                                      (security, shared_key, update_interval),
                                       1, 100)


    def _set_security_wpapsk(self, security, shared_key, update_interval=None):
        self.wait_for_object_by_id('security_type')
        if security == ap_spec.SECURITY_TYPE_WPAPSK:
            wpa_item = 'Enable WPA Wireless Security ( enhanced )'
        else:
            wpa_item = 'Enable WPA2 Wireless Security ( enhanced )'
        self.select_item_from_popup_by_id(wpa_item, 'security_type')
        self.set_content_of_text_field_by_id(shared_key, 'passphrase')
        self.set_content_of_text_field_by_id(shared_key, 'confirm_passphrase')


    def _set_visibility(self, visible=True):
        # value=0 is visible; value=1 is invisible
        int_value = not(int(visible))
        xpath = ('//input[@value="%d" and @name="ssid_broadcast"]' % int_value)
        self.click_button_by_xpath(xpath)
