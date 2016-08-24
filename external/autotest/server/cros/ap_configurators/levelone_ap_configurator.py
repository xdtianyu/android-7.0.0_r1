# Copyright (c) 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


import logging

import ap_spec
import dynamic_ap_configurator


class LevelOneAPConfigurator(
        dynamic_ap_configurator.DynamicAPConfigurator):
    """Base class for objects to configure Level one access points
       using webdriver."""

    def get_number_of_pages(self):
        return 2


    def get_supported_modes(self):
        return [{'band': ap_spec.BAND_2GHZ,
                 'modes': [ap_spec.MODE_B | ap_spec.MODE_G | ap_spec.MODE_N,
                           ap_spec.MODE_G, ap_spec.MODE_B, ap_spec.MODE_N]}]


    def get_supported_bands(self):
        return [{'band': ap_spec.BAND_2GHZ,
                 'channels': ['auto', 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11]}]


    def is_security_mode_supported(self, security_mode):
        return security_mode in (ap_spec.SECURITY_TYPE_DISABLED,
                                 ap_spec.SECURITY_TYPE_WPAPSK,
                                 ap_spec.SECURITY_TYPE_WPA2PSK,
                                 ap_spec.SECURITY_TYPE_WEP)


    def navigate_to_page(self, page_number):
        self.get_url(self.admin_interface_url, page_title='LevelOne')
        self.driver.switch_to_default_content()
        frame = self.wait_for_object_by_id('idleft')
        self.driver.switch_to_frame(frame)
        element = self.driver.find_element_by_link_text('Wireless')
        element.click()
        element = self.driver.find_element_by_link_text('Options')
        element.click()
        self.driver.switch_to_default_content()
        frame = self.wait_for_object_by_id('idmain')
        self.driver.switch_to_frame(frame)
        self.wait_for_object_by_xpath('//input[@name="ssid"]')
        if page_number == 2:
            self.click_button_by_xpath('//input[@name="wsec"]')
            self.wait_for_object_by_xpath('//select[@name="wsecurity"]')


    def save_page(self, page_number):
        self.click_button_by_xpath('//input[@name="save"]')
        if page_number == 1:
            self.wait_for_object_by_xpath('//input[@name="ssid"]')
        elif page_number == 2:
            self.wait_for_object_by_xpath('//select[@name="wsecurity"]')


    def set_mode(self, mode, band=None):
        self.add_item_to_command_list(self._set_mode, (mode,), 1, 900)


    def _set_mode(self, mode, band=None):
        mode_mapping = {
                ap_spec.MODE_B | ap_spec.MODE_G | ap_spec.MODE_N:'11b+g+n',
                ap_spec.MODE_G:'G Only', ap_spec.MODE_B:'B Only',
                ap_spec.MODE_N:'N Only', 'Disabled':'Off'}
        mode_name = mode_mapping.get(mode)
        if not mode_name:
            raise RuntimeError('The mode %d not supported by router %s. ',
                               hex(mode), self.name)
        xpath = '//select[@name="wire_mode"]'
        self.select_item_from_popup_by_xpath(mode_name, xpath)


    def set_ssid(self, ssid):
        self.add_item_to_command_list(self._set_ssid, (ssid,), 1, 900)


    def _set_ssid(self, ssid):
        xpath = '//input[@name="ssid"]'
        self.set_content_of_text_field_by_xpath(ssid, xpath, abort_check=False)
        self._ssid = ssid


    def set_channel(self, channel):
        self.add_item_to_command_list(self._set_channel, (channel,), 1, 900)


    def _set_channel(self, channel):
        position = self._get_channel_popup_position(channel)
        xpath = '//select[@name="w_channel"]'
        channels = ['auto', '01', '02', '03', '04', '05', '06', '07', '08',
                    '09', '10', '11']
        self.select_item_from_popup_by_xpath(channels[position], xpath)


    def set_radio(self, enabled=True):
        weight = 1 if enabled else 1000
        self.add_item_to_command_list(self._set_radio, (enabled,), 1, weight)


    def _set_radio(self, enabled=True):
        if not enabled:
            self._set_mode('Disabled')
        else:
            self._set_mode(ap_spec.MODE_G)


    def set_band(self, enabled=True):
        logging.debug('set_band is not supported in Linksys single band AP.')
        return None


    def set_security_disabled(self):
        self.add_item_to_command_list(self._set_security_disabled, (), 2, 1000)


    def _set_security_disabled(self):
        xpath = '//select[@name="wsecurity"]'
        self.select_item_from_popup_by_xpath('Disabled', xpath)


    def set_security_wep(self, key_value, authentication):
        self.add_item_to_command_list(self._set_security_wep,
                                      (key_value, authentication), 2, 1000)


    def _set_security_wep(self, key_value, authentication):
        popup = '//select[@name="wsecurity"]'
        if not self.item_in_popup_by_xpath_exist('WEP', popup):
            raise RuntimeError('The popup %s did not contain the item %s. '
                               'Is the mode N?' % (popup, self.security_wep))
        self.select_item_from_popup_by_xpath('WEP', popup)
        text = '//input[@name="passphrase"]'
        self.set_content_of_text_field_by_xpath(key_value, text,
                                                abort_check=True)
        xpath = '//input[@name="keygen"]'
        self.click_button_by_xpath(xpath)


    def set_security_wpapsk(self, security, shared_key, update_interval=None):
        self.add_item_to_command_list(self._set_security_wpapsk,
                                      (security, shared_key, update_interval),
                                       2, 900)


    def _set_security_wpapsk(self, security, shared_key, upadate_interval=None):
        """Common method to set wpapsk and wpa2psk modes."""
        popup = '//select[@name="wsecurity"]'
        self.wait_for_object_by_xpath(popup)
        if security == ap_spec.SECURITY_TYPE_WPAPSK:
            wpa_item = 'WPA-PSK'
            text = '//input[@name="wpa_psk"]'
        else:
            wpa_item = 'WPA2-PSK'
            text = '//input[@name="wpa2_psk"]'
        self.select_item_from_popup_by_xpath(wpa_item, popup)
        self.set_content_of_text_field_by_xpath(shared_key, text,
                                                abort_check=True)


    def is_update_interval_supported(self):
        return False


    def is_visibility_supported(self):
        return False
