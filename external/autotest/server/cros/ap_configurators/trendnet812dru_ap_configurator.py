# Copyright (c) 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import ap_spec
import trendnet692gr_ap_configurator
import time


class Trendnet812druAPConfigurator(trendnet692gr_ap_configurator.
                                  Trendnet692grAPConfigurator):
    """Derived class to control the Trendnet TEW-812DRU."""


    def _alert_handler(self, alert):
        """
        Checks for any modal dialogs which popup to alert the user and
        either raises a RuntimeError or ignores the alert.

        @param alert: The modal dialog's contents.
        """
        text = alert.text
        if 'WPS in Open security' in text:
            alert.accept()
        else:
            raise RuntimeError('An unexpected alert was thrown: %s' % text)


    def get_supported_modes(self):
        return [{'band': ap_spec.BAND_2GHZ, 'modes': [ap_spec.MODE_N]},
                {'band': ap_spec.BAND_5GHZ, 'modes': [ap_spec.MODE_N]}]


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
        """Navigates to the given page.

        @param page_number: the page to navigate to.
        """
        # All settings are on the same page, so we always open the config page
        if self.current_band == ap_spec.BAND_2GHZ:
            if page_number == 1:
                page_url = os.path.join(self.admin_interface_url ,
                                        'wireless/basic.asp?wl_unit=0')
            elif page_number == 2:
                page_url = os.path.join(self.admin_interface_url ,
                                        'wireless/security.asp?wl_unit=0')
            else:
                raise RuntimeError('Invalid page number passed. Number of pages'
                                   '%d, page value sent was %d' %
                                   (self.get_number_of_pages(), page_number))
        elif self.current_band == ap_spec.BAND_5GHZ:
            if page_number == 1:
                page_url = os.path.join(self.admin_interface_url ,
                                        'wireless/basic.asp?wl_unit=1')
            elif page_number == 2:
                page_url = os.path.join(self.admin_interface_url ,
                                        'wireless/security.asp?wl_unit=1')
            else:
                raise RuntimeError('Invalid page number passed. Number of pages'
                                   '%d, page value sent was %d' %
                                    (self.get_number_of_pages(), page_number))
        else:
            raise RuntimeError('Incorrect band band = %s' % self.current_band)
        self.get_url(page_url, page_title='TEW-812DRU')


    def _set_ssid(self, ssid):
        xpath = '//input[@maxlength="32" and @name="wl_ssid"]'
        self.set_content_of_text_field_by_xpath(ssid, xpath, abort_check=True)
        self._ssid = ssid


    def _set_mode(self, mode, band=None):
        # Different bands are not supported so we ignore.
        # Create the mode to popup item mapping
        mode_mapping = {ap_spec.MODE_N: 'Auto'}
        mode_name = ''
        if mode in mode_mapping.keys():
            mode_name = mode_mapping[mode]
        else:
            raise RuntimeError('The mode selected %s is not supported by router'
                               ' %s.', ap_spec.mode_string_for_mode(mode),
                               self.name)
        xpath = '//select[@name="wl_nmode"]'
        while self.number_of_items_in_popup_by_xpath(xpath) < 2:
            time.sleep(0.25)
        self.select_item_from_popup_by_xpath(mode_name, xpath)


    def set_radio(self, enabled=True):
        self.add_item_to_command_list(self._set_radio, (enabled, ), 1, 200)


    def _set_radio(self, enabled=True):
        xpath = '//select[@name="wl_bss_enabled"]'
        if enabled:
            self.select_item_from_popup_by_xpath('On', xpath)
        else:
            self.select_item_from_popup_by_xpath('Off', xpath)


    def _set_visibility(self, visible=True):
        xpath = '//select[@name="wl_closed"]'
        if visible:
            self.select_item_from_popup_by_xpath('Enabled', xpath)
        else:
            self.select_item_from_popup_by_xpath('Disabled', xpath)


    def _set_channel(self, channel):
        position = self._get_channel_popup_position(channel)
        xpath = '//select[@name="wl_chanspec"]'
        channel_choices_2GHZ = ['Auto', '1', '2', '3', '4', '5', '6', '7', '8',
                                '9', '10', '11']
        channel_choices_5GHZ = ['Auto', '36', '40', '44', '48', '149', '153',
                                '157', '161']
        if self.current_band == ap_spec.BAND_2GHZ:
            self.select_item_from_popup_by_xpath(channel_choices_2GHZ[position],
                                                 xpath)
        else:
            self.select_item_from_popup_by_xpath(channel_choices_5GHZ[position],
                                                 xpath)


    def _set_security_wpapsk(self, security, shared_key, update_interval=1800):
        self.wait_for_object_by_id('security_mode')
        if security == ap_spec.SECURITY_TYPE_WPAPSK:
            wpa_item = 'WPA-PSK'
        else:
            wpa_item = 'WPA2-PSK'
        self.select_item_from_popup_by_id(wpa_item, 'security_mode',
                                          wait_for_xpath='id("wpaPassphrase")')
        self.set_content_of_text_field_by_id(shared_key, 'wpaPassphrase')
        self.set_content_of_text_field_by_id(update_interval,
                                             'rotationInterval')
