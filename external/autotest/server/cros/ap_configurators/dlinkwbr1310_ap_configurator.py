# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Class to control the Dlink WBR1310AP router."""

import logging
import os
import time
import urlparse

import dynamic_ap_configurator
import ap_spec


class DLinkwbr1310APConfigurator(
        dynamic_ap_configurator.DynamicAPConfigurator):
    """Class to control the DLink wbr1310."""


    def _open_landing_page(self):
        page_url = urlparse.urljoin(self.admin_interface_url,'wireless.htm')
        self.get_url(page_url, page_title='D-LINK')
        pwd = '//input[@name="login_pass"]'
        if not self.object_by_xpath_exist(pwd):
            # We are at the config page, done.
            return

        xpath = '//input[@name="login_name"]'
        self.set_content_of_text_field_by_xpath('admin', xpath,
                                                abort_check=False)
        self.set_content_of_text_field_by_xpath('password', pwd,
                                                abort_check=False)
        self.click_button_by_xpath('//input[@name="login"]')
        wlan = '//a[text()="Wireless settings"]'
        self.wait_for_object_by_xpath(wlan)
        self.click_button_by_xpath(wlan)


    def get_supported_bands(self):
        return [{'band': ap_spec.BAND_2GHZ,
                 'channels': [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11]}]


    def get_supported_modes(self):
        return [{'band': ap_spec.BAND_2GHZ,
                 'modes': [ap_spec.MODE_G, ap_spec.MODE_B]}]


    def get_number_of_pages(self):
        return 1


    def is_security_mode_supported(self, security_mode):
        """
        Returns if a given security_type is supported.

        @param security_mode: one security modes defined in the APSpec

        @return True if the security mode is supported; False otherwise.

        """
        return security_mode in (ap_spec.SECURITY_TYPE_DISABLED,
                                 ap_spec.SECURITY_TYPE_WPAPSK,
                                 ap_spec.SECURITY_TYPE_WPA2PSK,
                                 ap_spec.SECURITY_TYPE_WEP)


    def navigate_to_page(self, page_number):
        """
        Navigates to the page corresponding to the given page number.

        This method performs the translation between a page number and a url to
        load. This is used internally by apply_settings.

        @param page_number: page number of the page to load

        """
        # All settings are on the same page, so we always open the config page
        self._open_landing_page()


    def save_page(self, page_number):
        """
        Saves the given page.

        @param page_number: Page number of the page to save.

        """
        # All settings are on the same page, we can ignore page_number
        self.click_button_by_xpath('//input[@name="button"]')
        progress_value = self.wait_for_object_by_id("wTime")
        # Give the router 40 secs to update.
        for i in xrange(80):
            page_name = os.path.basename(self.driver.current_url)
            time.sleep(0.5)
            if page_name == 'wireless.htm':
                break

    def is_update_interval_supported(self):
        """
        Returns True if setting the PSK refresh interval is supported.

        @return True is supported; False otherwise
        """
        return False

    def set_mode(self, mode_enable=True, band=None):
        self.add_item_to_command_list(self._set_mode, (mode_enable,), 1, 900)


    def _set_mode(self, mode_enable=True):
        # For dlinkwbr1310, 802.11g is the only available mode.
        logging.debug('This router (%s) does not support multiple modes.',
                      self.name)
        return None


    def set_radio(self, enabled=True):
        logging.debug('This router (%s) does not support radio.', self.name)
        return None


    def set_ssid(self, ssid):
        self.add_item_to_command_list(self._set_ssid, (ssid,), 1, 900)


    def _set_ssid(self, ssid):
        self.set_content_of_text_field_by_id(ssid, 'ssid')
        self._ssid = ssid


    def set_channel(self, channel):
        self.add_item_to_command_list(self._set_channel, (channel,), 1, 900)


    def _set_channel(self, channel):
        position = self._get_channel_popup_position(channel)
        channel_ch = ['1', '2', '3', '4', '5', '6', '7', '8', '9', '10', '11']
        xpath = '//select[@name="channel"]'
        self.select_item_from_popup_by_xpath(channel_ch[position], xpath)


    def set_band(self, band):
        logging.debug('This router (%s) does not support multiple bands.',
                      self.name)
        return None


    def set_security_disabled(self):
        self.add_item_to_command_list(self._set_security_disabled, (), 1, 900)


    def _set_security_disabled(self):
        security_disabled = 'Disable Wireless Security (not recommended)'
        self.select_item_from_popup_by_id(security_disabled, 'wep_type')


    def set_security_wep(self, key_value, authentication):
        self.add_item_to_command_list(self._set_security_wep,
                                      (key_value, authentication), 1, 900)


    def _set_security_wep(self, key_value, authentication):
        popup = '//select[@name="wep_type"]'
        self.wait_for_object_by_xpath(popup)
        security_wep = 'Enable WEP Wireless Security (basic)'
        self.select_item_from_popup_by_xpath(security_wep, popup)
        key_type = '//select[@name="wep_key_type"]'
        self.select_item_from_popup_by_xpath('ASCII', key_type)
        text_field = '//input[@name="key1"]'
        self.set_content_of_text_field_by_xpath(key_value, text_field,
                                                abort_check=True)


    def set_security_wpapsk(self, security, shared_key, update_interval=None):
        self.add_item_to_command_list(self._set_security_wpapsk,
                                      (security, shared_key, update_interval),
                                       1, 900)


    def _set_security_wpapsk(self, security, shared_key, update_interval=None):
        popup = '//select[@name="wep_type"]'
        self.wait_for_object_by_xpath(popup)
        key_field1 = '//input[@name="wpapsk1"]'
        key_field2 = '//input[@name="wpapsk2"]'
        if security == ap_spec.SECURITY_TYPE_WPAPSK:
            wpa_item = 'Enable WPA-Personal Wireless Security (enhanced)'
        else:
            wpa_item = 'Enable WPA2 Wireless Security (enhanced)'
        self.select_item_from_popup_by_xpath(wpa_item, popup,
                                             wait_for_xpath=key_field1)
        self.set_content_of_text_field_by_xpath(shared_key, key_field1,
                                                abort_check=False)
        self.set_content_of_text_field_by_xpath(shared_key, key_field2,
                                                abort_check=False)


    def set_visibility(self, visible=True):
        self.add_item_to_command_list(self._set_visibility, (visible,), 1, 900)


    def _set_visibility(self, visible=True):
        xpath = '//input[@name="ssidBroadcast"]'
        self.set_check_box_selected_by_xpath(xpath, selected=True)
