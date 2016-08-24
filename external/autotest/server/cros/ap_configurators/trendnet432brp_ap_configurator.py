# Copyright (c) 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


import os

import ap_spec
import trendnet_ap_configurator


class Trendnet432brpAPConfigurator(trendnet_ap_configurator.
                                   TrendnetAPConfigurator):
    """Derived class to control the Trendnet TEW-432BRP."""


    def _alert_handler(self, alert):
        """
        Checks for any modal dialogs which popup to alert the user and
        either raises a RuntimeError or ignores the alert.

        @param alert: The modal dialog's contents.
        """
        text = alert.text
        if 'The length of Key1 must be 10 characters' in text:
            alert.accept()
        elif 'Are you sure you want to continue' in text:
            alert.accept()
        else:
            raise RuntimeError(text)


    def get_number_of_pages(self):
        return 2


    def get_supported_bands(self):
        return [{'band': ap_spec.BAND_2GHZ, 'channels': range(1, 13)}]


    def get_supported_modes(self):
        return [{'band': ap_spec.BAND_2GHZ, 'modes': [ap_spec.MODE_G]}]


    def is_security_mode_supported(self, security_mode):
        """Returns in the given security mode is supported.

        @param security_mode: a security mode that is define in APSpec.
        """
        return security_mode in (ap_spec.SECURITY_TYPE_DISABLED,
                                 ap_spec.SECURITY_TYPE_WEP,
                                 ap_spec.SECURITY_TYPE_WPAPSK,
                                 ap_spec.SECURITY_TYPE_WPA2PSK)


    def navigate_to_page(self, page_number):
        """Navigates to the given pages.

        @param page_number: the page to navigate to.
        """
        if page_number == 1:
            page_url = os.path.join(self.admin_interface_url,'wlan_basic.asp')
            self.get_url(page_url, page_title='TEW-432BRP')
        elif page_number == 2:
            page_url = os.path.join(self.admin_interface_url,
                                    'wlan_security.asp')
            self.get_url(page_url, page_title='TEW-432BRP')
        else:
            raise RuntimeError('Invalid page number passed.  Number of pages '
                               '%d, page value sent was %d' %
                               (self.get_number_of_pages(), page_number))


    def save_page(self, page_number):
        """Saves the given page.

        @param page_number: the page to save.
        """
        xpath = ('//input[@name="apply" and @value="Apply"]')
        self.click_button_by_xpath(xpath, alert_handler=self._alert_handler)


    def set_radio(self, enabled=True):
        self.add_item_to_command_list(self._set_radio, (enabled, ), 1, 1000)


    def _set_radio(self, enabled=True):
        # value=0 is ON; value=1 is OFF
        int_value = not(int(enabled))
        xpath = ('//input[@value="%d" and @name="enable"]' % int_value)
        self.click_button_by_xpath(xpath, alert_handler=self._alert_handler)


    def _set_ssid(self, ssid):
        self.set_content_of_text_field_by_id(ssid, 'ssid')
        self._ssid = ssid


    def _set_channel(self, channel):
        position = self._get_channel_popup_position(channel)
        channel_choices = ['1', '2', '3', '4', '5', '6', '7'
                           '8', '9', '10', '11', '12', '13']
        self.select_item_from_popup_by_id(channel_choices[position], 'channel')


    def set_band(self, band):
        return None


    def set_mode(self, mode, band=None):
        return None


    def set_security_disabled(self):
        self.add_item_to_command_list(self._set_security_disabled, (), 2, 1000)


    def _set_security_disabled(self):
        self.wait_for_object_by_id('wep_type')
        self.select_item_from_popup_by_id(' Disable ', 'wep_type')


    def set_security_wep(self, key_value, authentication):
        self.add_item_to_command_list(self._set_security_wep,
                                      (key_value, authentication), 2, 900)


    def _set_security_wep(self, key_value, authentication):
        self.wait_for_object_by_id('wep_type')
        self.select_item_from_popup_by_id(' WEP ', 'wep_type')
        self.select_item_from_popup_by_id('ASCII', 'wep_key_type')
        self.select_item_from_popup_by_id(' 64-bit', 'wep_key_len')
        self.set_content_of_text_field_by_id(key_value, 'key1',
                                             abort_check=True)


    def _set_security_wpapsk(self, security, shared_key, update_interval=None):
        self.wait_for_object_by_id('wep_type')
        if security == ap_spec.SECURITY_TYPE_WPAPSK:
            self.select_item_from_popup_by_id(' WPA ', 'wep_type')
        else:
            self.select_item_from_popup_by_id(' WPA2 ', 'wep_type')
        self.set_content_of_text_field_by_id(shared_key, 'wpapsk1')
        self.set_content_of_text_field_by_id(shared_key, 'wpapsk2')


    def _set_visibility(self, visible=True):
        # value=0 is visible; value=1 is invisible
        int_value = not(int(visible))
        xpath = ('//input[@value="%d" and @name="ssid_broadcast"]' % int_value)
        self.click_button_by_xpath(xpath)
