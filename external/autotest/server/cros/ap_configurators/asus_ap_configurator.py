# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Class to control the AsusAP router."""

import logging
from selenium.common.exceptions import TimeoutException

import dynamic_ap_configurator
import ap_spec


class AsusAPConfigurator(
        dynamic_ap_configurator.DynamicAPConfigurator):
    """Base class for Asus RT-N56U router."""


    def _set_authentication(self, authentication, wait_for_xpath=None):
        """Sets the authentication method in the popup.

        Args:
          authentication: The authentication method to select.
          wait_for_path: An item to wait for before returning.
        """
        auth = '//select[@name="rt_auth_mode"]'
        if self.current_band == ap_spec.BAND_5GHZ:
            auth = '//select[@name="wl_auth_mode"]'
        self.select_item_from_popup_by_xpath(authentication, auth,
            wait_for_xpath, alert_handler=self._invalid_security_handler)


    def _invalid_security_handler(self, alert):
        text = alert.text
        # This tweaks encryption but is more of a warning, so we can dismiss.
        if text.find('will change WEP or TKIP encryption to AES') != -1:
            alert.accept()
        else:
            raise RuntimeError('You have entered an invalid configuration: '
                               '%s' % text)


    def get_number_of_pages(self):
        return 2


    def get_supported_bands(self):
        return [{'band': ap_spec.BAND_2GHZ,
                 'channels': [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11]},
                {'band': ap_spec.BAND_5GHZ,
                 'channels': [36, 40, 44, 48, 149, 153, 157, 161]}]


    def get_supported_modes(self):
        return [{'band': ap_spec.BAND_2GHZ,
                 'modes': [ap_spec.MODE_B, ap_spec.MODE_N, ap_spec.MODE_B |
                           ap_spec.MODE_G, ap_spec.MODE_B]},
                {'band': ap_spec.BAND_5GHZ,
                 'modes': [ap_spec.MODE_N, ap_spec.MODE_A]}]


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
        # The page is determined by what band we are using. We ignore the input.
        admin_url = self.admin_interface_url
        if self.current_band == ap_spec.BAND_2GHZ:
            self.get_url('%s/Advanced_Wireless2g_Content.asp' % admin_url,
                         page_title='2.4G')
        elif self.current_band == ap_spec.BAND_5GHZ:
            self.get_url('%s/Advanced_Wireless_Content.asp' % admin_url,
                         page_title='5G')
        else:
            raise RuntimeError('Invalid page number passed.  Number of pages '
                               '%d, page value sent was %d' %
                               (self.get_number_of_pages(), page_number))


    def save_page(self, page_number):
        """
        Saves the given page.

        @param page_number: Page number of the page to save.

        """
        button = self.driver.find_element_by_id('applyButton')
        button.click()
        menu_id = 'menu_body' #  id of the table with the main content
        try:
            self.wait_for_object_by_id(menu_id)
        except TimeoutException, e:
            raise RuntimeError('Unable to find the object by id: %s\n '
                               'WebDriver exception: %s' % (menu_id, str(e)))
        self.navigate_to_page(page_number)


    def set_mode(self, mode, band=None):
        #  To avoid the modal dialog
        self.add_item_to_command_list(self._set_security_disabled, (), 1, 799)
        self.add_item_to_command_list(self._set_mode, (mode, band), 1, 800)


    def _set_mode(self, mode, band=None):
        xpath = '//select[@name="rt_gmode"]'
        #  Create the mode to popup item mapping
        mode_mapping = {ap_spec.MODE_B: 'b Only', ap_spec.MODE_G: 'g Only',
                        ap_spec.MODE_B | ap_spec.MODE_G: 'b/g Only',
                        ap_spec.MODE_N: 'n Only', ap_spec.MODE_A: 'a Only'}
        mode_name = ''
        if self.current_band == ap_spec.BAND_5GHZ or band == ap_spec.BAND_5GHZ:
            xpath = '//select[@name="wl_gmode"]'
        if mode in mode_mapping.keys():
            mode_name = mode_mapping[mode]
            if ((mode & ap_spec.MODE_A) and
                (self.current_band == ap_spec.BAND_2GHZ)):
                #  a mode only in 5Ghz
                logging.debug('Mode \'a\' is not available for 2.4Ghz band.')
                return
            elif ((mode & (ap_spec.MODE_B | ap_spec.MODE_G) ==
                  (ap_spec.MODE_B | ap_spec.MODE_G)) or
                 (mode & ap_spec.MODE_B == ap_spec.MODE_B) or
                 (mode & ap_spec.MODE_G == ap_spec.MODE_G)) and \
                 (self.current_band != ap_spec.BAND_2GHZ):
                #  b/g, b, g mode only in 2.4Ghz
                logging.debug('Mode \'%s\' is not available for 5Ghz band.',
                             mode_name)
                return
        else:
            raise RuntimeError('The mode selected \'%s\' is not supported by '
                               'router %s.' % mode_name, self.name)
        self.select_item_from_popup_by_xpath(mode_name, xpath,
            alert_handler=self._invalid_security_handler)


    def set_radio(self, enabled):
        #  We cannot turn off radio on ASUS.
        return None


    def set_ssid(self, ssid):
        self.add_item_to_command_list(self._set_ssid, (ssid,), 1, 900)


    def _set_ssid(self, ssid):
        xpath = '//input[@maxlength="32" and @name="rt_ssid"]'
        if self.current_band == ap_spec.BAND_5GHZ:
            xpath = '//input[@maxlength="32" and @name="wl_ssid"]'
        self.set_content_of_text_field_by_xpath(ssid, xpath)
        self._ssid = ssid


    def set_channel(self, channel):
        self.add_item_to_command_list(self._set_channel, (channel,), 1, 900)


    def _set_channel(self, channel):
        position = self._get_channel_popup_position(channel)
        channel_choices = range(1, 12)
        xpath = '//select[@name="rt_channel"]'
        if self.current_band == ap_spec.BAND_5GHZ:
            xpath = '//select[@name="wl_channel"]'
            channel_choices = ['36', '40', '44', '48', '149', '153',
                               '157', '161']
        self.select_item_from_popup_by_xpath(str(channel_choices[position]),
                                             xpath)


    def set_band(self, band):
        if band == ap_spec.BAND_2GHZ:
            self.current_band = ap_spec.BAND_2GHZ
        elif band == ap_spec.BAND_5GHZ:
            self.current_band = ap_spec.BAND_5GHZ
        else:
            raise RuntimeError('Invalid band sent %s' % band)


    def set_security_disabled(self):
        self.add_item_to_command_list(self._set_security_disabled, (), 1, 1000)


    def _set_security_disabled(self):
        popup = '//select[@name="rt_wep_x"]'
        if self.current_band == ap_spec.BAND_5GHZ:
            popup = '//select[@name="wl_wep_x"]'
        self._set_authentication('Open System', wait_for_xpath=popup)
        self.select_item_from_popup_by_xpath('None', popup)


    def set_security_wep(self, key_value, authentication):
        self.add_item_to_command_list(self._set_security_wep,
                                      (key_value, authentication), 1, 1000)


    def _set_security_wep(self, key_value, authentication):
        popup = '//select[@name="rt_wep_x"]'
        text_field = '//input[@name="rt_phrase_x"]'
        if self.current_band == ap_spec.BAND_5GHZ:
            popup = '//select[@name="wl_wep_x"]'
            text_field = '//input[@name="wl_phrase_x"]'
        self._set_authentication('Open System', wait_for_xpath=popup)
        self.select_item_from_popup_by_xpath('WEP-64bits', popup,
                                             wait_for_xpath=text_field,
                                             alert_handler=
                                             self._invalid_security_handler)
        self.set_content_of_text_field_by_xpath(key_value, text_field,
                                                abort_check=True)


    def set_security_wpapsk(self, security, shared_key, update_interval=1800):
        #  Asus does not support TKIP (wpapsk) encryption in 'n' mode.
        #  So we will use AES (wpa2psk) to avoid conflicts and modal dialogs.
        self.add_item_to_command_list(self._set_security_wpapsk,
                                      (security, shared_key, update_interval),
                                       1, 900)


    def _set_security_wpapsk(self, security, shared_key, update_interval):
        key_field = '//input[@name="rt_wpa_psk"]'
        interval_field = '//input[@name="rt_wpa_gtk_rekey"]'
        if self.current_band == ap_spec.BAND_5GHZ:
            key_field = '//input[@name="wl_wpa_psk"]'
            interval_field = '//input[@name="wl_wpa_gtk_rekey"]'
        if security == ap_spec.SECURITY_TYPE_WPAPSK:
            self._set_authentication('WPA-Personal',
                                     wait_for_xpath=key_field)
        else:
            self._set_authentication('WPA2-Personal',
                                     wait_for_xpath=key_field)
        self.set_content_of_text_field_by_xpath(shared_key, key_field)
        self.set_content_of_text_field_by_xpath(str(update_interval),
                                                interval_field)


    def set_visibility(self, visible=True):
        self.add_item_to_command_list(self._set_visibility,(visible,), 1, 900)


    def _set_visibility(self, visible=True):
        #  value=0 is visible; value=1 is invisible
        value = 0 if visible else 1
        xpath = '//input[@name="rt_closed" and @value="%s"]' % value
        if self.current_band == ap_spec.BAND_5GHZ:
            xpath = '//input[@name="wl_closed" and @value="%s"]' % value
        self.click_button_by_xpath(xpath,
                                   alert_handler=self._invalid_security_handler)
