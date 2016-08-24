# Copyright (c) 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Class to control the Dlink_DIR300AP router."""

import urlparse
import logging

import dlink_ap_configurator
import ap_spec

class DLinkDIR300APConfigurator(
        dlink_ap_configurator.DLinkAPConfigurator):
    """Derived class to control the Dlink_DIR300AP router."""


    def get_number_of_pages(self):
        return 1;


    def get_supported_bands(self):
        return [{'band': ap_spec.BAND_2GHZ,
                 'channels': [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11]}]


    def get_supported_modes(self):
        return [{'band': ap_spec.BAND_2GHZ,
                 'modes': [ap_spec.MODE_G]}]


    def is_security_mode_supported(self, security_mode):
        """
        Returns if a given security_type is supported.

        @param security_mode: one security modes defined in the APSpec

        @return True if the security mode is supported; False otherwise.

        """
        return security_mode in (ap_spec.SECURITY_TYPE_DISABLED,
                                 ap_spec.SECURITY_TYPE_WEP,
                                 ap_spec.SECURITY_TYPE_WPAPSK,
                                 ap_spec.SECURITY_TYPE_WPA2PSK)


    def navigate_to_page(self, page_number):
        """
        Navigates to the page corresponding to the given page number.

        This method performs the translation between a page number and a url to
        load. This is used internally by apply_settings.

        @param page_number: page number of the page to load

        """
        page_url = urlparse.urljoin(self.admin_interface_url, 'bsc_wlan.php')
        self.get_url(page_url, page_title='D-LINK SYSTEMS, INC | '
                    'WIRELESS ROUTER | HOME')
        pwd = '//input[@name="LOGIN_PASSWD"]'
        if not self.object_by_xpath_exist(pwd):
            # We are at the config page, done.
            return
        xpath = '//input[@name="LOGIN_USER"]'
        self.set_content_of_text_field_by_xpath('admin', xpath,
                                                abort_check=True)
        self.set_content_of_text_field_by_xpath('password', pwd,
                                                abort_check=True)
        self.click_button_by_xpath('//input[@name="login"]')


    def save_page(self, page_number):
        """
        Saves the given page.

        @param page_number: Page number of the page to save.

        """
        # All settings are on the same page, we can ignore page_number
        button = self.driver.find_element_by_xpath('//input[@name="apply"]')
        button.click()
        # If we did not make changes we are sent to the continue screen.
        button_xpath = '//input[@name="bt"]'
        if self.object_by_xpath_exist(button_xpath):
            button = self.driver.find_element_by_xpath(button_xpath)
            button.click()
        # We will be returned to the landing page when complete
        self.wait_for_object_by_id('enable')


    def set_mode(self, mode, band=None):
        # Mode overrides the band.  So if a band change is made after a mode
        # change it may make an incompatible pairing.
        logging.debug('This router (%s) does not support multiple modes.',
                      self.name)
        return None


    def set_radio(self, enabled=True):
        # If we are enabling we are activating all other UI components, do
        # it first. Otherwise we are turning everything off so do it last.
        if enabled:
            weight = 1
        else:
            weight = 1000
        self.add_item_to_command_list(self._set_radio, (enabled,), 1, weight)


    def _set_radio(self, enabled=True):
        # The radio checkbox for this router always has a value of 1. So we need
        # to use other methods to determine if the radio is on or not. Check if
        # the ssid textfield is disabled.
        ssid = self.driver.find_element_by_xpath('//input[@id="ssid"]')
        if ssid.get_attribute('disabled') == 'true':
            radio_enabled = False
        else:
            radio_enabled = True
        if radio_enabled == enabled:
            # Nothing to do
            return
        self.set_check_box_selected_by_id('enable', selected=False)


    def set_ssid(self, ssid):
        # Can be done as long as it is enabled
        self.add_item_to_command_list(self._set_ssid, (ssid,), 1, 900)


    def _set_ssid(self, ssid):
        self._set_radio(enabled=True)
        self._ssid = ssid
        self.set_content_of_text_field_by_id(ssid, 'ssid')


    def set_channel(self, channel):
        self.add_item_to_command_list(self._set_channel, (channel,), 1, 900)


    def _set_channel(self, channel):
        position = self._get_channel_popup_position(channel)
        self._set_radio(enabled=True)
        channel_choices = ['1 ', '2', '3', '4', '5', '6', '7', '8', '9', '10',
                           '11']
        channel_popup = self.driver.find_element_by_id('channel')
        if channel_popup.get_attribute('disabled') == 'true':
            self.set_check_box_selected_by_id('autochann', selected=False)
        self.select_item_from_popup_by_id(channel_choices[position], 'channel')


    def set_band(self, band):
        logging.debug('This router (%s) does not support multiple bands.',
                      self.name)
        return None


    def set_security_disabled(self):
        self.add_item_to_command_list(self._set_security_disabled, (), 1, 900)


    def _set_security_disabled(self):
        self._set_radio(enabled=True)
        security_disabled = 'Disable Wireless Security (not recommended)'
        self.select_item_from_popup_by_id(security_disabled, 'security_type')


    def set_security_wep(self, key_value, authentication):
        self.add_item_to_command_list(self._set_security_wep,
                                      (key_value, authentication), 1, 900)


    def _set_security_wep(self, key_value, authentication):
        self._set_radio(enabled=True)
        security_wep = 'Enable WEP Wireless Security (basic)'
        self.select_item_from_popup_by_id(security_wep, 'security_type',
                                          wait_for_xpath='id("wepkey_64")')
        self.select_item_from_popup_by_id('64Bit', 'wep_key_len',
                                          wait_for_xpath='id("wepkey_64")')
        self.set_content_of_text_field_by_id(key_value, 'wepkey_64')


    def set_security_wpapsk(self, security, shared_key, update_interval=1800):
        self.add_item_to_command_list(self._set_security_wpapsk,
                                     (security, shared_key, update_interval),
                                      1, 900)


    def _set_security_wpapsk(self, security, shared_key, update_interval=1800):
        self._set_radio(enabled=True)
        if security == ap_spec.SECURITY_TYPE_WPAPSK:
            wpa_item = 'Enable WPA Only Wireless Security (enhanced)'
        else:
            wpa_item = 'Enable WPA2 Only Wireless Security (enhanced)'
        self.select_item_from_popup_by_id(wpa_item, 'security_type')
        self.select_item_from_popup_by_id('PSK', 'psk_eap')
        self.set_content_of_text_field_by_id(shared_key, 'wpapsk1')


    def set_visibility(self, visible=True):
        self.add_item_to_command_list(self._set_visibility, (visible,), 1, 900)


    def _set_visibility(self, visible=True):
        self._set_radio(enabled=True)
        found_id = self.wait_for_objects_by_id(['aphidden'], wait_time=20)
        if ('aphidden' in found_id) and visible:
            self.set_check_box_selected_by_id('aphidden', selected=True)
        elif ('aphidden' in found_id) and not visible:
            self.set_check_box_selected_by_id('aphidden', selected=False)
        else:
            raise RuntimeError('Unable to set visibility.')
