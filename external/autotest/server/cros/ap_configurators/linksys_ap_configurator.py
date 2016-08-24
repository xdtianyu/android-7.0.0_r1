# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Class to control the Linksys WRT54G2 router."""

import logging
import urlparse

import dynamic_ap_configurator
import ap_spec


class LinksysAPConfigurator(
        dynamic_ap_configurator.DynamicAPConfigurator):
    """Derived class to control Linksys WRT54G2 router."""


    def _sec_alert(self, alert):
        text = alert.text
        if 'Invalid Key, must be between 8 and 63 ASCII' in text:
            raise RuntimeError('We got a modal dialog. ' + text)
        else:
            raise RuntimeError('Unhandled alert message: %s' % text)


    def get_number_of_pages(self):
        return 2


    def get_supported_bands(self):
        return [{'band': ap_spec.BAND_2GHZ,
                 'channels': [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11]}]


    def get_supported_modes(self):
        return [{'band': ap_spec.BAND_2GHZ,
                 'modes': [ap_spec.MODE_B, ap_spec.MODE_G, ap_spec.MODE_B |
                           ap_spec.MODE_G]}]


    def is_security_mode_supported(self, security_mode):
        return security_mode in (ap_spec.SECURITY_TYPE_DISABLED,
                                 ap_spec.SECURITY_TYPE_WPAPSK,
                                 ap_spec.SECURITY_TYPE_WPA2PSK,
                                 ap_spec.SECURITY_TYPE_WEP)


    def navigate_to_page(self, page_number):
        if page_number == 1:
            url = urlparse.urljoin(self.admin_interface_url, 'wireless.htm')
            self.driver.get(url)
            xpath = '//input[@name="wsc_smode" and @value=1]'
            button = self.driver.find_element_by_xpath(xpath)
            if not button.is_selected():
                self.click_button_by_xpath(xpath)
        elif page_number == 2:
            url = urlparse.urljoin(self.admin_interface_url, 'WSecurity.htm')
            self.driver.get(url)
        else:
            raise RuntimeError('Invalid page number passed.  Number of pages '
                               '%d, page value sent was %d' %
                               (self.get_number_of_pages(), page_number))


    def save_page(self, page_number):
        self.wait_for_object_by_id('divBT1')
        self.click_button_by_xpath('id("divBT1")')
        # Wait for the continue button
        continue_xpath = '//input[@value="Continue" and @type="button"]'
        self.wait_for_object_by_xpath(continue_xpath)
        self.click_button_by_xpath(continue_xpath,
                                   alert_handler=self._sec_alert)


    def set_mode(self, mode, band=None):
        self.add_item_to_command_list(self._set_mode, (mode,), 1, 900)


    def _set_mode(self, mode):
        # Different bands are not supported so we ignore.
        # Create the mode to popup item mapping
        mode_mapping = {ap_spec.MODE_B: 'B-Only', ap_spec.MODE_G: 'G-Only',
                        ap_spec.MODE_B | ap_spec.MODE_G: 'Mixed',
                        'Disabled': 'Disabled'}
        mode_name = mode_mapping.get(mode)
        if not mode_name:
            raise RuntimeError('The mode selected %d is not supported by router'
                               ' %s.', hex(mode), self.name)
        xpath = ('//select[@onchange="SelWL()" and @name="Mode"]')
        self.select_item_from_popup_by_xpath(mode_name, xpath)


    def set_radio(self, enabled=True):
        # If we are enabling we are activating all other UI components, do it
        # first.  Otherwise we are turning everything off so do it last.
        weight = 1 if enabled else 1000
        self.add_item_to_command_list(self._set_radio, (enabled,), 1, weight)


    def _set_radio(self, enabled=True):
        # To turn off we pick disabled, to turn on we set to G
        if not enabled:
            self._set_mode('Disabled')
        else:
            self._set_mode(ap_spec.MODE_G)


    def set_ssid(self, ssid):
        self.add_item_to_command_list(self._set_ssid, (ssid,), 1, 900)


    def _set_ssid(self, ssid):
        self._set_radio(enabled=True)
        xpath = ('//input[@maxlength="32" and @name="SSID"]')
        self.set_content_of_text_field_by_xpath(ssid, xpath)
        self._ssid = ssid


    def set_channel(self, channel):
        self.add_item_to_command_list(self._set_channel, (channel,), 1, 900)


    def _set_channel(self, channel):
        position = self._get_channel_popup_position(channel)
        self._set_radio(enabled=True)
        channel_choices = ['1 - 2.412GHz', '2 - 2.417GHz', '3 - 2.422GHz',
                           '4 - 2.427GHz', '5 - 2.432GHz', '6 - 2.437GHz',
                           '7 - 2.442GHz', '8 - 2.447GHz', '9 - 2.452GHz',
                           '10 - 2.457GHz', '11 - 2.462GHz']
        xpath = ('//select[@onfocus="check_action(this,0)" and @name="Freq"]')
        self.select_item_from_popup_by_xpath(channel_choices[position],
                                             xpath)


    def set_band(self, band):
        return None


    def set_security_disabled(self):
        self.add_item_to_command_list(self._set_security_disabled, (), 2, 1000)


    def _set_security_disabled(self):
        xpath = ('//select[@name="SecurityMode"]')
        self.select_item_from_popup_by_xpath('Disabled', xpath)


    def set_security_wep(self, key_value, authentication):
        self.add_item_to_command_list(self._set_security_wep,
                                      (key_value, authentication), 2, 1000)


    def _set_security_wep(self, key_value, authentication):
        logging.debug('This router %s doesnt support WEP authentication type: '
                      '%s', self.name, authentication)
        popup = '//select[@name="SecurityMode"]'
        self.wait_for_object_by_xpath(popup)
        text_field = ('//input[@name="wl_passphrase"]')
        self.select_item_from_popup_by_xpath('WEP', popup,
                                             wait_for_xpath=text_field,
                                             alert_handler=self._sec_alert)
        self.set_content_of_text_field_by_xpath(key_value, text_field,
                                                abort_check=True)
        self.click_button_by_xpath('//input[@value="Generate"]',
                                   alert_handler=self._sec_alert)


    def set_security_wpapsk(self, security, shared_key, update_interval=1800):
        self.add_item_to_command_list(self._set_security_wpapsk,
                                      (security, shared_key, update_interval),
                                       2, 900)


    def _set_security_wpapsk(self, security, shared_key, update_interval=1800):
        if update_interval < 600:
            logging.debug('The minimum update interval is 600, overriding.')
            update_interval = 600
        elif update_interval > 7200:
            logging.debug('The maximum update interval is 7200, overriding.')
            update_interval = 7200
        popup = '//select[@name="SecurityMode"]'
        self.wait_for_object_by_xpath(popup)
        key_field = '//input[@name="PassPhrase"]'
        if security == ap_spec.SECURITY_TYPE_WPAPSK:
            wpa_item = 'WPA Personal'
        else:
            wpa_item = 'WPA2 Personal'
        self.select_item_from_popup_by_xpath(wpa_item, popup,
                                             wait_for_xpath=key_field,
                                             alert_handler=self._sec_alert)
        self.set_content_of_text_field_by_xpath(shared_key, key_field,
                                                abort_check=True)
        interval_field = ('//input[@name="GkuInterval"]')
        self.set_content_of_text_field_by_xpath(str(update_interval),
                                                interval_field)


    def set_visibility(self, visible=True):
        self.add_item_to_command_list(self._set_visibility, (visible,), 1, 900)


    def _set_visibility(self, visible=True):
        self._set_radio(enabled=True)
        # value=1 is visible; value=0 is invisible
        int_value = int(visible)
        xpath = ('//input[@value="%d" and @name="wl_closed"]' % int_value)
        self.click_button_by_xpath(xpath)
