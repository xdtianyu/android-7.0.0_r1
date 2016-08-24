# Copyright (c) 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


import linksyse2100_ap_configurator
import ap_spec

from selenium.common.exceptions import WebDriverException


class LinksysWRT54GLAPConfigurator(
        linksyse2100_ap_configurator.Linksyse2100APConfigurator):
    """Base class for objects to configure Linksys WRT 54GL access points
       using webdriver."""


    def get_supported_modes(self):
        return [{'band': ap_spec.BAND_2GHZ,
                 'modes': [ap_spec.MODE_M, ap_spec.MODE_B, ap_spec.MODE_G]}]


    def get_supported_bands(self):
        return [{'band': ap_spec.BAND_2GHZ,
                 'channels': [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11]}]


    def save_page(self, page_number):
        submit_btn = '//a[@href="javascript:to_submit(document.forms[0])"]'
        continue_btn = '//input[@value="Continue" and @onclick="to_submit()"]'
        try:
            self.click_button_by_xpath(submit_btn,
                                       alert_handler=self._sec_alert)
            self.wait_for_object_by_xpath(continue_btn, wait_time=10)
            self.click_button_by_xpath(continue_btn,
                                       alert_handler=self._sec_alert)
        except WebDriverException, e:
            self._check_for_alert_in_message(str(e),
                                             alert_handler=self._sec_alert)


    def _set_mode(self, mode, band=None):
        mode_mapping = {ap_spec.MODE_M:'Mixed', ap_spec.MODE_G:'G-Only',
                        ap_spec.MODE_B:'B-Only', 'Disabled':'Disabled'}
        mode_name = mode_mapping.get(mode)
        if not mode_name:
            raise RuntimeError('The mode %d not supported by router %s. ',
                               hex(mode), self.name)
        xpath = '//select[@name="wl_net_mode"]'
        self.select_item_from_popup_by_xpath(mode_name, xpath,
                                             alert_handler=self._sec_alert)



    def _set_channel(self, channel):
        position = self._get_channel_popup_position(channel)
        xpath = '//select[@name="wl_channel"]'
        channels = ['1 - 2.412GHZ', '2 - 2.417GHZ', '3 - 2.422GHZ',
                    '4 - 2.427GHZ', '5 - 2.432GHZ', '6 - 2.437GHZ',
                    '7 - 2.442GHZ', '8 - 2.447GHZ', '9 - 2.452GHZ',
                    '10 - 2.457GHZ', '11 - 2.462GHZ']
        self.select_item_from_popup_by_xpath(channels[position], xpath)


    def _set_security_wpapsk(self, security, shared_key, update_interval=None):
        """Common method to set wpapsk and wpa2psk modes."""
        popup = '//select[@name="security_mode2"]'
        self.wait_for_object_by_xpath(popup)
        if security == ap_spec.SECURITY_TYPE_WPAPSK:
            wpa_item = 'WPA Personal'
        else:
            wpa_item = 'WPA2 Personal'
        self.select_item_from_popup_by_xpath(wpa_item, popup,
                                             alert_handler=self._sec_alert)
        text = '//input[@name="wl_wpa_psk"]'
        self.set_content_of_text_field_by_xpath(shared_key, text,
                                                abort_check=True)
        interval = '//input[@name="wl_wpa_gtk_rekey"]'
        if update_interval:
            self.set_content_of_text_field_by_xpath(interval, update_interval,
                                                    abort_check=True)


    def is_update_interval_supported(self):
        """
        Returns True if setting the PSK refresh interval is supported.

        @return True is supported; False otherwise
        """
        return True
