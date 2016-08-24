# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Baseclass to control Netgear single band routers."""

import logging
import urlparse
import dynamic_ap_configurator
import ap_spec

from selenium.common.exceptions import TimeoutException as \
    SeleniumTimeoutException

class NetgearSingleBandAPConfigurator(
        dynamic_ap_configurator.DynamicAPConfigurator):
    """Baseclass to control Netgear single band routers."""


    def _alert_handler(self, alert):
        """Checks for any modal dialogs which popup to alert the user and
        either raises a RuntimeError or ignores the alert.

        Args:
          alert: The modal dialog's contents.
        """
        text = alert.text
        raise RuntimeError('We have an unhandeled alert. %s' % text)


    def get_number_of_pages(self):
        return 1


    def is_update_interval_supported(self):
        """
        Returns True if setting the PSK refresh interval is supported.

        @return True is supported; False otherwise
        """
        return False


    def get_supported_bands(self):
        return [{'band': ap_spec.BAND_2GHZ,
                 'channels': ['Auto', 1, 2, 3, 4, 5, 6, 7, 8, 9 , 10, 11]}]


    def get_supported_modes(self):
        return [{'band': ap_spec.BAND_2GHZ,
                 'modes': [ap_spec.MODE_G, ap_spec.MODE_B | ap_spec.MODE_G]}]


    def is_security_mode_supported(self, security_mode):
        return security_mode in (ap_spec.SECURITY_TYPE_DISABLED,
                                 ap_spec.SECURITY_TYPE_WPAPSK,
                                 ap_spec.SECURITY_TYPE_WPA2PSK,
                                 ap_spec.SECURITY_TYPE_WEP)


    def navigate_to_page(self, page_number):
        self.get_url(urlparse.urljoin(self.admin_interface_url,
                     'WLG_wireless.htm'))
        try:
            self.wait_for_object_by_xpath('//input[@name="ssid"]')
        except SeleniumTimeoutException, e:
            raise SeleniumTimeoutException('Unable to navigate to settings '
                                           'page. WebDriver exception:%s', e)


    def save_page(self, page_number):
        self.click_button_by_xpath('//input[@name="Apply"]',
                                   alert_handler=self._alert_handler)


    def set_radio(self, enabled=True):
        logging.debug('set_radio is not supported in this router.')
        return None


    def set_ssid(self, ssid):
        self.add_item_to_command_list(self._set_ssid, (ssid,), 1, 900)


    def _set_ssid(self, ssid):
        xpath = '//input[@maxlength="32" and @name="ssid"]'
        self.set_content_of_text_field_by_xpath(ssid, xpath, abort_check=True)
        self._ssid = ssid


    def set_channel(self, channel):
        self.add_item_to_command_list(self._set_channel, (channel,), 1, 900)


    def _set_channel(self, channel):
        position = self._get_channel_popup_position(channel)
        channel_choices = ['Auto', '01', '02', '03', '04', '05', '06',
                           '07', '08', '09', '10', '11']
        xpath = '//select[@name="w_channel"]'
        self.select_item_from_popup_by_xpath(channel_choices[position], xpath)


    def set_mode(self, mode, band=None):
        self.add_item_to_command_list(self._set_mode, (mode,), 1, 900)


    def _set_mode(self, mode, band=None):
        if mode == ap_spec.MODE_G:
            mode_popup = 'g only'
        elif mode == (ap_spec.MODE_G | ap_spec.MODE_B):
            mode_popup = 'b and g'
        else:
            raise RuntimeError('Invalid mode passed %x.' % mode)
        xpath = '//select[@name="opmode"]'
        self.select_item_from_popup_by_xpath(mode_popup, xpath)


    def set_band(self, band):
        logging.debug('The router has just one band.')
        return None


    def set_security_disabled(self):
        self.add_item_to_command_list(self._set_security_disabled, (), 1, 900)


    def _set_security_disabled(self):
        xpath = '//input[@name="security_type" and @value="Disable"]'
        self.click_button_by_xpath(xpath, alert_handler=self._alert_handler)


    def set_security_wep(self, value, authentication):
        self.add_item_to_command_list(self._set_security_wep,
                                     (value, authentication), 1, 900)


    def _set_security_wep(self, value, authentication):
        xpath = '//input[@name="security_type" and @value="WEP"]'
        try:
            self.click_button_by_xpath(xpath, alert_handler=self._alert_handler)
        except Exception, e:
            raise RuntimeError('We got an exception: "%s". Check the mode. '
                               'It should be \'Up to 54 Mbps\'.' % str(e))
        xpath = '//input[@name="passphraseStr"]'
        self.set_content_of_text_field_by_xpath(value, xpath, abort_check=True)
        xpath = '//input[@value="Generate"]'
        self.click_button_by_xpath(xpath, alert_handler=self._alert_handler)


    def set_security_wpapsk(self, security, key, update_interval=None):
        self.add_item_to_command_list(self._set_security_wpapsk, (security,
                                      key,), 1, 900)


    def _set_security_wpapsk(self, security, key, update_interval=None):
        # Update Interval is not supported.
        if security == ap_spec.SECURITY_TYPE_WPAPSK:
            xpath = '//input[@name="security_type" and @value="WPA-PSK"]'
        else:
            xpath = '//input[@name="security_type" and @value="WPA2-PSK"]'
        self.click_button_by_xpath(xpath, alert_handler=self._alert_handler)
        xpath = '//input[@name="passphrase"]'
        self.set_content_of_text_field_by_xpath(key, xpath, abort_check=True)


    def is_visibility_supported(self):
        """
        Returns if AP supports setting the visibility (SSID broadcast).

        @return True if supported; False otherwise.
        """
        return False
