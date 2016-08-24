# Copyright (c) 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Class to control the Buffalo WZR d1800hAP router."""

import logging
import urlparse

import dynamic_ap_configurator
import ap_spec


class BuffalowzrAPConfigurator(
        dynamic_ap_configurator.DynamicAPConfigurator):
    """Base class for Buffalo WZR router."""


    def get_number_of_pages(self):
        return 2


    def is_update_interval_supported(self):
        """
        Returns True if setting the PSK refresh interval is supported.

        @return True is supported; False otherwise
        """
        return False


    def get_supported_modes(self):
        return [{'band': ap_spec.BAND_2GHZ,
                 'modes': [ap_spec.MODE_B, ap_spec.MODE_N, ap_spec.MODE_G]},
                {'band': ap_spec.BAND_5GHZ,
                 'modes': [ap_spec.MODE_N, ap_spec.MODE_A]}]


    def get_supported_bands(self):
        return [{'band': ap_spec.BAND_2GHZ,
                 'channels': ['Auto', 1, 2, 3, 4, 5, 6, 7, 8, 9 , 10, 11]},
                {'band': ap_spec.BAND_5GHZ,
                 'channels': ['Auto', 36, 40, 44, 48, 149, 153,
                              157, 161, 165]}]


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
        if page_number == 1:
            url = 'cgi-bin/cgi?req=frm&frm=top_wizard_func_wlan_channel.html'
            page_url = urlparse.urljoin(self.admin_interface_url, url)
            self.get_url(page_url, page_title='AirStation Settings')
        elif page_number == 2:
            url = 'cgi-bin/cgi?req=frm&frm=top_wizard_func_wlan_if.html'
            page_url = urlparse.urljoin(self.admin_interface_url, url)
            self.get_url(page_url, page_title='AirStation Settings')
        else:
            raise RuntimeError('Invalid page number passed. Number of pages '
                               '%d, page value sent was %d' %
                               (self.get_number_of_pages(), page_number))


    def refresh_page(self, apply_set):
        """Refresh the settings page

        @param apply_set: xpath for the apply button.

        """
        self.driver.refresh()
        self._switch_frame()
        self.wait_for_object_by_xpath(apply_set, wait_time=40)


    def save_page(self, page_number):
        """
        Saves the given page.

        @param page_number: Page number of the page to save.

        """
        self._switch_frame()
        apply_set = '//input[@type="submit"]'
        try:
            self.wait_for_object_by_xpath(apply_set, wait_time=40)
            self.click_button_by_xpath(apply_set)
        except:
            self.refresh_page(apply_set)
            self.click_button_by_xpath(apply_set)
        # We need to hit one more apply button when settings have changed.
        try:
            if self.driver.find_element_by_xpath(apply_set):
                self.click_button_by_xpath(apply_set)
        except:
            logging.debug('Settings have not been changed.')
        complete = '//input[@type="button"]'
        # Give some time for router to save changes.
        # In case when contents of page are not displayed, we need
        # to refresh page and click apply button one more time.
        try:
            self.wait_for_object_by_xpath(complete, wait_time=40)
        except:
            self.refresh_page(apply_set)
            self.click_button_by_xpath(apply_set)
        self.wait_for_object_by_xpath(complete, wait_time=40)
        self.driver.find_element_by_xpath(complete)
        self.click_button_by_xpath(complete)


    def _switch_frame(self):
        frame1 = self.driver.find_element_by_xpath('//frame[@name="lower"]')
        self.driver.switch_to_frame(frame1)


    def set_mode(self, mode, band=None):
        # We cannot set mode in Buffalo WZR.
        logging.debug('This router (%s) does not support setting mode.' ,
                      self.name)
        return None


    def set_radio(self, enabled=True):
        #  We cannot turn off radio on Buffalo WZR.
        logging.debug('This router (%s) does not support radio.' , self.name)
        return None


    def set_ssid(self, ssid):
        self.add_item_to_command_list(self._set_ssid, (ssid,), 1, 900)


    def _set_ssid(self, ssid):
        self._switch_frame()
        xpath = '//input[@type="text" and @name="ssid_11bg"]'
        if self.current_band == ap_spec.BAND_5GHZ:
            xpath = '//input[@type="text" and @name="ssid_11a"]'
        self.set_content_of_text_field_by_xpath(ssid, xpath)
        default = self.driver.switch_to_default_content()
        self._ssid = ssid


    def set_channel(self, channel):
        self.add_item_to_command_list(self._set_channel, (channel,), 1, 900)


    def _set_channel(self, channel):
        apply_set = '//input[@type="submit"]'
        self._switch_frame()
        position = self._get_channel_popup_position(channel)
        channel_choice = ['Auto', 'Channel 1', 'Channel 2', 'Channel 3',
                          'Channel 4', 'Channel 5', 'Channel 6', 'Channel 7',
                          'Channel 8', 'Channel 9', 'Channel 10', 'Channel 11']
        xpath = '//select[@name="channel11bg"]'
        if self.current_band == ap_spec.BAND_5GHZ:
            xpath = '//select[@name="channel11a"]'
            channel_choice = ['Auto', 'Channel 36', 'Channel 40', 'Channel 44',
                              'Channel 48', 'Channel 149', 'Channel 153',
                              'Channel 157', 'Channel 161', 'Channel 165']
        try:
            if self.number_of_items_in_popup_by_xpath(xpath) == 0:
                # If the popup is empty, refresh.
                self.refresh_page(apply_set)
        except:
            self.refresh_page(apply_set)
        self.select_item_from_popup_by_xpath(channel_choice[position], xpath)
        default = self.driver.switch_to_default_content()


    def set_ch_width(self, width):
        """
        Adjusts the channel width.

        @param width: the channel width
        """
        self.add_item_to_command_list(self._set_ch_width,(width,), 1, 900)


    def _set_ch_width(self, width):
        self._switch_frame()
        channel_width_choice = ['11n/g/bNormal Mode (20 MHz)',
                                '11n/g/b450 Mbps Mode (40 MHz)']
        xpath = '//select[@name="nbw_11bg"]'
        if self.current_band == ap_spec.BAND_5GHZ:
            channel_width_choice = ['11n/aNormal Mode (20 MHz)',
                                    '11n/a450 Mbps Mode (40 MHz)',
                                    '11ac/n/a1300 Mbps Mode (80 MHz)']
            xpath = '//select[@name="nbw_11a"]'
        self.select_item_from_popup_by_xpath(channel_width_choice[width],
                                             xpath)
        default = self.driver.switch_to_default_content()


    def set_band(self, band):
        if band == ap_spec.BAND_5GHZ:
            self.current_band = ap_spec.BAND_5GHZ
        elif band == ap_spec.BAND_2GHZ:
            self.current_band = ap_spec.BAND_2GHZ
        else:
            raise RuntimeError('Invalid band sent %s' % band)


    def set_security_disabled(self):
        self.add_item_to_command_list(self._set_security_disabled, (), 2, 1000)


    def _set_security_disabled(self):
        self._switch_frame()
        xpath = '//span[@class="WLAN11G"]'
        if self.current_band == ap_spec.BAND_5GHZ:
            xpath = '//span[@class="WLAN11A"]'
        self.driver.find_element_by_xpath(xpath)
        self.click_button_by_xpath(xpath)
        self.driver.find_element_by_xpath('//a[text()="Open"]')
        self.click_button_by_xpath('//a[text()="Open"]')
        default = self.driver.switch_to_default_content()


    def set_security_wep(self, key_value, authentication):
        self.add_item_to_command_list(self._set_security_wep,
                                      (key_value, authentication), 2, 1000)


    def _set_security_wep(self, key_value, authentication):
        self.security_wep = "Character Input : 5 characters (WEP64)"
        self._switch_frame()
        xpath = '//span[@class="WLAN11G"]'
        if self.current_band == ap_spec.BAND_5GHZ:
            xpath = '//span[@class="WLAN11A"]'
        self.driver.find_element_by_xpath(xpath)
        self.click_button_by_xpath(xpath)
        self.driver.find_element_by_xpath('//a[text()="WEP"]')
        self.click_button_by_xpath('//a[text()="WEP"]')
        popup = '//select[@name="weptype"]'
        text_field = '//input[@name="key0"]'
        self.wait_for_object_by_xpath(popup)
        self.select_item_from_popup_by_xpath(self.security_wep, popup,
                                             wait_for_xpath=text_field)
        self.set_content_of_text_field_by_xpath(key_value, text_field,
                                                abort_check=True)
        default = self.driver.switch_to_default_content()


    def set_security_wpapsk(self, security, shared_key, update_interval=None):
        self.add_item_to_command_list(self._set_security_wpapsk,
                                      (security, shared_key,), 2, 900)


    def _set_security_wpapsk(self, security, shared_key, update_interval=None):
        self._switch_frame()
        xpath = '//span[@class="WLAN11G"]'
        if self.current_band == ap_spec.BAND_5GHZ:
            xpath = '//span[@class="WLAN11A"]'
        self.driver.find_element_by_xpath(xpath)
        self.click_button_by_xpath(xpath)
        if security == ap_spec.SECURITY_TYPE_WPAPSK:
            wpa_item = '//a[text()="WPA-PSK (AES)"]'
        else:
            wpa_item = '//a[text()="WPA2-PSK (AES)"]'
        self.driver.find_element_by_xpath(wpa_item)
        self.click_button_by_xpath(wpa_item)
        text_field = '//input[@name="wpapsk"]'
        self.set_content_of_text_field_by_xpath(shared_key, text_field,
                                                abort_check=True)
        default = self.driver.switch_to_default_content()


    def is_visibility_supported(self):
        """
        Returns if AP supports setting the visibility (SSID broadcast).

        @return True if supported; False otherwise.
        """
        return False
