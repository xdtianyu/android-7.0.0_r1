# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

import ap_spec
import netgear_WNDR_dual_band_configurator


class NetgearR6200APConfigurator(netgear_WNDR_dual_band_configurator.
                                NetgearDualBandAPConfigurator):
    """Derived class to control Netgear R6200 router."""


    def _alert_handler(self, alert):
        """Checks for any modal dialogs which popup to alert the user and
        either raises a RuntimeError or ignores the alert.

        @param alert: The modal dialog's contents.
        """
        text = alert.text
        if 'WPS requires SSID broadcasting in order to work' in text:
            alert.accept()
        else:
            super(NetgearR6200APConfigurator, self)._alert_handler(alert)


    def navigate_to_page(self, page_number):
        """Navigates to the default page.

        @param page_number: The page to load.
        """
        super(NetgearR6200APConfigurator, self).navigate_to_page(page_number)
        self.set_wait_time(30)
        self.wait.until(lambda _:
                self.driver.execute_script('return document.readyState')
                == 'complete')
        self.restore_default_wait_time()
        xpath = '//select[@name="opmode"]'
        if not self.item_in_popup_by_xpath_exist('Up to 54 Mbps', xpath):
            raise RuntimeError('Router webpage did not load completely.')


    def get_supported_modes(self):
        return [{'band': ap_spec.BAND_2GHZ,
                 'modes': [ap_spec.MODE_B, ap_spec.MODE_G, ap_spec.MODE_N]},
                {'band': ap_spec.BAND_5GHZ,
                 'modes': [ap_spec.MODE_G, ap_spec.MODE_A, ap_spec.MODE_N]}]


    def get_supported_bands(self):
        return [{'band': ap_spec.BAND_2GHZ,
                 'channels': ['Auto', 1, 2, 3, 4, 5, 6, 7, 8, 9 , 10, 11]},
                {'band': ap_spec.BAND_5GHZ,
                 'channels': [36, 40, 44, 48, 149, 153, 157, 161]}]


    def is_security_mode_supported(self, security_mode):
        """Returns if the passed in security mode is supported.

        @param security_mode: a security mode that is defined in APSpec
        """
        return security_mode in (ap_spec.SECURITY_TYPE_DISABLED,
                                 ap_spec.SECURITY_TYPE_WPA2PSK,
                                 ap_spec.SECURITY_TYPE_WEP)


    def set_channel(self, channel):
        self.add_item_to_command_list(self._set_channel, (channel,), 1, 900)


    def _set_channel(self, channel):
        position = self._get_channel_popup_position(channel)
        channel_choices = ['Auto', '01', '02', '03', '04', '05', '06', '07',
                           '08', '09', '10', '11']
        xpath = '//select[@name="w_channel"]'
        if self.current_band == ap_spec.BAND_5GHZ:
           xpath = '//select[@name="w_channel_an"]'
           channel_choices = ['36', '40', '44', '48', '149', '153',
                              '157', '161']
        self.select_item_from_popup_by_xpath(channel_choices[position],
                                             xpath)


    def _set_mode(self, mode, band=None):
        router_mode = None
        xpath = '//select[@name="opmode"]'
        if self.current_band == ap_spec.BAND_2GHZ:
            if mode == ap_spec.MODE_B:
                router_mode = 'Up to 54 Mbps'
            elif mode == ap_spec.MODE_G:
                router_mode = 'Up to 145 Mbps'
            elif mode == ap_spec.MODE_N:
                router_mode = 'Up to 300 Mbps'
        elif self.current_band == ap_spec.BAND_5GHZ:
            xpath = '//select[@name="opmode_an"]'
            if mode == ap_spec.MODE_G:
                router_mode = 'Up to 173 Mbps'
            elif mode == ap_spec.MODE_A:
                router_mode = 'Up to 400 Mbps'
            elif mode == ap_spec.MODE_N:
                router_mode = 'Up to 867 Mbps'
        if not router_mode:
            raise RuntimeError('You selected a mode that is not assigned '
                               'to this router. Select either b, g or n '
                               'for 2.4Ghz or either g, a or n for 5Ghz.')
        self.wait_for_object_by_xpath(xpath, wait_time=10)
        self.select_item_from_popup_by_xpath(router_mode, xpath)


    def set_security_wep(self, key_value, authentication):
        if self.current_band == ap_spec.BAND_5GHZ:
            logging.debug('Cannot set WEP security for 5GHz band in Netgear '
                          'R6200 router.')
            return None
        super(NetgearR6200APConfigurator, self).set_security_wep(
                key_value, authentication)
