# Copyright (c) 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


import ap_spec
import linksyse2500_ap_configurator


class LinksysWRT610NAPConfigurator(linksyse2500_ap_configurator.
                                   Linksyse2500APConfigurator):
    """Base class for objects to configure Linksys wrt610 and E3000 AP's
       using webdriver."""


    def is_security_mode_supported(self, security_mode):
        """Returns if the passed in security mode is supported.

        @param security_mode: one of the supported security methods defined
                              in APSpec.

        @returns True is suppported; False otherwise.
        """
        return security_mode in (ap_spec.SECURITY_TYPE_DISABLED,
                                 ap_spec.SECURITY_TYPE_WPAPSK,
                                 ap_spec.SECURITY_TYPE_WPA2PSK,
                                 ap_spec.SECURITY_TYPE_WEP)


    def _set_mode(self, mode, band=None):
        mode_mapping = {ap_spec.MODE_B: 'Wireless-B Only',
                        ap_spec.MODE_G: 'Wireless-G Only',
                        ap_spec.MODE_B | ap_spec.MODE_G: 'Wireless-B/G Only',
                        ap_spec.MODE_N: 'Wireless-N Only',
                        ap_spec.MODE_A: 'Wireless-A Only',
                        ap_spec.MODE_M: 'Mixed'}
        xpath = '//select[@name="wl0_net_mode"]'
        if self.current_band == ap_spec.BAND_5GHZ:
            xpath = '//select[@name="wl1_net_mode"]'
        mode_name = ''
        if mode in mode_mapping.keys():
            mode_name = mode_mapping[mode]
        else:
            raise RuntimeError('The mode selected %d is not supported by router'
                               ' %s.', hex(mode), self.name)
        self.select_item_from_popup_by_xpath(mode_name, xpath,
                                             alert_handler=self._alert_handler)


    def _set_ssid(self, ssid):
        xpath = '//input[@name="wl0_ssid"]'
        if self.current_band == ap_spec.BAND_5GHZ:
            xpath = '//input[@name="wl1_ssid"]'
        self.set_content_of_text_field_by_xpath(ssid, xpath)
        self._ssid = ssid


    def _set_channel(self, channel):
        position = self._get_channel_popup_position(channel)
        channel_choices = ['Auto',
                           '1 - 2.412GHZ', '2 - 2.417GHZ', '3 - 2.422GHZ',
                           '4 - 2.427GHZ', '5 - 2.432GHZ', '6 - 2.437GHZ',
                           '7 - 2.442GHZ', '8 - 2.447GHZ', '9 - 2.452GHZ',
                           '10 - 2.457GHZ', '11 - 2.462GHZ']
        xpath = '//select[@name="_wl0_channel"]'
        if self.current_band == ap_spec.BAND_5GHZ:
            xpath = '//select[@name="_wl1_channel"]'
            channel_choices = ['Auto (DFS)',
                               '36 - 5.180GHz', '40 - 5.200GHz',
                               '44 - 5.220GHz', '48 - 5.240GHz',
                               '149 - 5.745GHz', '153 - 5.765GHz',
                               '157 - 5.785GHz', '161 - 5.805GHz']
        self.select_item_from_popup_by_xpath(channel_choices[position], xpath,
                                             alert_handler=self._alert_handler)


    def _set_visibility(self, visible=True):
        button = 'wl0_closed'
        if self.current_band == ap_spec.BAND_5GHZ:
            button = 'wl1_closed'
        int_value = 0 if visible else 1
        xpath = ('//input[@value="%d" and @name="%s"]' % (int_value, button))
        self.click_button_by_xpath(xpath, alert_handler=self._alert_handler)
