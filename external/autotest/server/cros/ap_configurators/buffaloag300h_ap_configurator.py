# Copyright(c) 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


import logging
import urlparse

import ap_spec
import dynamic_ap_configurator


class Buffaloag300hAPConfigurator(dynamic_ap_configurator.
                                  DynamicAPConfigurator):
    """Base class for Buffalo WZR AG300H router."""


    def get_number_of_pages(self):
        return 2


    def is_update_interval_supported(self):
        return True


    def get_supported_modes(self):
        return [{'band': ap_spec.BAND_2GHZ,
                 'modes': [ap_spec.MODE_B, ap_spec.MODE_N, ap_spec.MODE_G,
                           ap_spec.MODE_N | ap_spec.MODE_G, ap_spec.MODE_M,
                           ap_spec.MODE_B | ap_spec.MODE_G]},
                {'band': ap_spec.BAND_5GHZ,
                 'modes': [ap_spec.MODE_N, ap_spec.MODE_A, ap_spec.MODE_M]}]


    def get_supported_bands(self):
        return [{'band': ap_spec.BAND_2GHZ,
                 'channels': ['Auto', 1, 2, 3, 4, 5, 6, 7, 8, 9 , 10, 11]},
                {'band': ap_spec.BAND_5GHZ,
                 'channels': ['Auto', 36, 40, 44, 48, 149, 153,
                              157, 161, 165]}]


    def is_security_mode_supported(self, security_mode):
        return security_mode in (ap_spec.SECURITY_TYPE_DISABLED,
                                 ap_spec.SECURITY_TYPE_WPAPSK,
                                 ap_spec.SECURITY_TYPE_WPA2PSK,
                                 ap_spec.SECURITY_TYPE_WEP)


    def navigate_to_page(self, page_number):
        if page_number == 1:
            page_url = urlparse.urljoin(self.admin_interface_url,
                                        'Wireless_Basic.asp')
            self.get_url(page_url, page_title='Wireless')
        elif page_number == 2:
            page_url = urlparse.urljoin(self.admin_interface_url,
                                        'WL_WPATable.asp')
            self.get_url(page_url, page_title='Security')
        else:
            raise RuntimeError('Invalid page number passed. Number of pages '
                               '%d, page value sent was %d' %
                               (self.get_number_of_pages(), page_number))


    def save_page(self, page_number):
        apply_set = '//input[@name="save_button"]'
        self.click_button_by_xpath(apply_set)


    def set_mode(self, mode, band=None):
        self.add_item_to_command_list(self._set_mode, (mode, band,), 1, 900)


    def _set_mode(self, mode, band=None):
        mode_mapping = {ap_spec.MODE_B: 'B-Only',
                        ap_spec.MODE_G: 'G-Only',
                        ap_spec.MODE_B | ap_spec.MODE_G: 'BG-Mixed',
                        ap_spec.MODE_N | ap_spec.MODE_G: 'NG-Mixed',
                        ap_spec.MODE_N: 'N-Only (2.4 GHz)',
                        ap_spec.MODE_A: 'A-Only',
                        ap_spec.MODE_M: 'Mixed'}
        xpath = '//select[@name="ath0_net_mode"]'
        if self.current_band == ap_spec.BAND_5GHZ:
            xpath = '//select[@name="ath1_net_mode"]'
            mode_mapping[ap_spec.MODE_N] = 'N-Only (5 GHz)'
        mode_name = ''
        if mode in mode_mapping.keys():
            mode_name = mode_mapping[mode]
        else:
            raise RuntimeError('The mode selected %d is not supported by router'
                               ' %s.', hex(mode), self.name)
        self.select_item_from_popup_by_xpath(mode_name, xpath)


    def set_radio(self, enabled=True):
        #  We cannot turn off radio on Buffalo WZR.
        logging.debug('This router (%s) does not support radio.' , self.name)
        return None


    def set_ssid(self, ssid):
        self.add_item_to_command_list(self._set_ssid, (ssid,), 1, 900)


    def _set_ssid(self, ssid):
        xpath = '//input[@name="ath0_ssid"]'
        if self.current_band == ap_spec.BAND_5GHZ:
            xpath = '//input[@name="ath1_ssid"]'
        self.set_content_of_text_field_by_xpath(ssid, xpath)
        self._ssid = ssid


    def set_channel(self, channel):
        self.add_item_to_command_list(self._set_channel, (channel,), 1, 900)


    def _set_channel(self, channel):
        position = self._get_channel_popup_position(channel)
        channel_choices = ['Auto',
                           '1 - 2412 MHz', '2 - 2417 MHz', '3 - 2422 MHz',
                           '4 - 2427 MHz', '5 - 2432 MHz', '6 - 2437 MHz',
                           '7 - 2442 MHz', '8 - 2447 MHz', '9 - 2452 MHz',
                           '10 - 2457 MHz', '11 - 2462 MHz']
        xpath = '//select[@name="ath0_channel"]'
        if self.current_band == ap_spec.BAND_5GHZ:
            xpath = '//select[@name="ath1_channel"]'
            channel_choices = ['Auto', '36 - 5180 MHz', '40 - 5200 MHz',
                               '44 - 5220 MHz', '48 - 5240 MHz',
                               '149 - 5745 MHz', '153 - 5765 MHz',
                               '157 - 5785 MHz', '161 - 5805 MHz']
        self.select_item_from_popup_by_xpath(channel_choices[position], xpath)


    def set_band(self, band):
        if band == ap_spec.BAND_5GHZ:
            self.current_band = ap_spec.BAND_5GHZ
        elif band == ap_spec.BAND_2GHZ:
            self.current_band = ap_spec.BAND_2GHZ
        else:
            raise RuntimeError('Invalid band sent %s' % band)


    def set_security_disabled(self):
        self.add_item_to_command_list(self._set_security_disabled, (), 2, 1000)


    def _set_security(self, mode, wait_for_xpath=None):
        xpath = '//select[@name="ath0_security_mode"]'
        if self.current_band == ap_spec.BAND_5GHZ:
            xpath = '//select[@name="ath1_security_mode"]'
        self.wait_for_object_by_xpath(xpath)
        self.select_item_from_popup_by_xpath(mode, xpath,
                                             wait_for_xpath=wait_for_xpath)


    def _set_security_disabled(self):
        self._set_security('Disabled')


    def set_security_wep(self, key_value, authentication):
        self.add_item_to_command_list(self._set_security_wep,
                                      (key_value, authentication), 2, 1000)


    def _set_security_wep(self, key_value, authentication):
        text_field = '//input[@name="ath0_passphrase"]'
        xpath = '//input[@name="ath0_key1"]'
        if self.current_band == ap_spec.BAND_5GHZ:
            text_field = '//input[@name="ath1_passphrase"]'
            xpath = '//input[@name="ath1_key1"]'
        self._set_security('WEP', wait_for_xpath=text_field)
        button = '//input[@name="wepGenerate"]'
        generate_list = self.driver.find_elements_by_xpath(button)
        generate = generate_list[0]
        if self.current_band == ap_spec.BAND_5GHZ and len(generate_list) > 1:
            generate = generate_list[1]
        self.set_content_of_text_field_by_xpath(key_value, text_field,
                                                abort_check=True)
        generate.click()
        self.wait_for_object_by_xpath(xpath)


    def set_security_wpapsk(self, security, shared_key, update_interval=None):
        self.add_item_to_command_list(self._set_security_wpapsk,
                                (security, shared_key,update_interval), 2, 900)


    def _set_security_wpapsk(self, security, shared_key, update_interval=None):
        key_field = '//input[@name="ath0_wpa_psk"]'
        interval = '//input[@name="ath0_wpa_gtk_rekey"]'
        if self.current_band == ap_spec.BAND_5GHZ:
            key_field = '//input[@name="ath1_wpa_psk"]'
            interval = '//input[@name="ath1_wpa_gtk_rekey"]'
        if security == ap_spec.SECURITY_TYPE_WPAPSK:
            wpa_item = 'WPA Personal'
        else:
            wpa_item = 'WPA2 Personal'
        self._set_security(wpa_item, wait_for_xpath=key_field)
        self.set_content_of_text_field_by_xpath(shared_key, key_field,
                                                abort_check=True)
        if update_interval:
            self.set_content_of_text_field_by_xpath(update_interval, interval,
                                                    abort_check=True)


    def set_visibility(self, visible=True):
        self.add_item_to_command_list(self._set_visibility, (visible,), 1, 900)


    def _set_visibility(self, visible=True):
        button = 'ath0_closed'
        if self.current_band == ap_spec.BAND_5GHZ:
            button = 'ath1_closed'
        int_value = 0 if visible else 1
        xpath = ('//input[@value="%d" and @name="%s"]' % (int_value, button))
        self.click_button_by_xpath(xpath)
