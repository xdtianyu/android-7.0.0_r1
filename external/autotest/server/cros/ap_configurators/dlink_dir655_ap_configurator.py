# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Class to control the Dlink Dir655 router."""

import logging
import time
import urlparse

import dynamic_ap_configurator
import ap_spec
from selenium.common.exceptions import TimeoutException as \
    SeleniumTimeoutException


class DLinkDIR655APConfigurator(
        dynamic_ap_configurator.DynamicAPConfigurator):
    """Derived class to control the DLink DIR-655."""
    first_login = True

    def _alert_handler(self, alert):
        """Checks for any modal dialogs which popup to alert the user and
        either raises a RuntimeError or ignores the alert.

        Args:
          alert: The modal dialog's contents.
        """
        text = alert.text
        if 'Password Invalid' in text:
            alert.accept()
        elif 'Nothing has changed, save anyway?' in text:
            alert.accept()
        elif 'Mode to 802.11n only, while there is an SSID with WEP' in text:
            alert.accept()
            raise RuntimeError('Security modes are not compatible: %s' % text)
        elif 'The Radius Server 1 can not be zero.' in text:
            alert.accept()
            raise RuntimeError('Invalid configuration, alert message:\n%s'
                               % text)
        elif 'The length of the Passphrase must be at least' in text:
            alert.accept()
            raise RuntimeError('Invalid configuration, alert message:\n%s'
                               % text)
        elif 'Invalid password, please try again' in text:
            alert.accept()
            if self.first_login:
                self.first_login = False
                self.login_to_ap()
        else:
            alert.accept()
            raise RuntimeError('We have an unhandled alert: %s' % text)


    def get_number_of_pages(self):
        return 1


    def is_update_interval_supported(self):
        """
        Returns True if setting the PSK refresh interval is supported.

        @return True is supported; False otherwise
        """
        return True


    def get_supported_bands(self):
        return [{'band': ap_spec.BAND_2GHZ,
                 'channels': [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11]}]


    def get_supported_modes(self):
        return [{'band': ap_spec.BAND_2GHZ,
                 'modes': [ap_spec.MODE_B, ap_spec.MODE_G, ap_spec.MODE_N,
                           ap_spec.MODE_B | ap_spec.MODE_G,
                           ap_spec.MODE_G | ap_spec.MODE_N,
                           ap_spec.MODE_B | ap_spec.MODE_G | ap_spec.MODE_N]}]


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
        # All settings are on the same page, so we always open the config page
        page_url = urlparse.urljoin(self.admin_interface_url, 'wireless.asp')
        self.get_url(page_url, page_title='D-LINK CORPORATION')
        # We wait for the page to load and avoid the intermediate page
        found_id = self.wait_for_objects_by_id(['w_enable', 'log_pass'],
                                               wait_time=30)
        if 'log_pass' in found_id:
            self.login_to_ap()
        elif 'w_enable' not in found_id:
            raise Exception(
                    'Unable to navigate to login or configuration page.')


    def login_to_ap(self):
        """Logs into the AP."""
        self.set_content_of_text_field_by_id('password', 'log_pass')
        self.click_button_by_id('login', alert_handler=self._alert_handler)
        # This will send us to the landing page and not where we want to go.
        page_url = urlparse.urljoin(self.admin_interface_url, 'wireless.asp')
        self.get_url(page_url, page_title='D-LINK CORPORATION')


    def save_page(self, page_number):
        """
        Saves the given page.

        @param page_number: Page number of the page to save.

        """
        # All settings are on the same page, we can ignore page_number
        self.click_button_by_id('button', alert_handler=self._alert_handler)
        # Give the router a minute to update.
        for i in xrange(120):
            progress_value = self.wait_for_object_by_id('show_sec')
            html = self.driver.execute_script('return arguments[0].innerHTML',
                                              progress_value)
            time.sleep(0.5)
            if int(html) == 0:
                break
        self.click_button_by_id('button', alert_handler=self._alert_handler)
        self.wait_for_object_by_id('w_enable')


    def set_mode(self, mode, band=None):
        # Mode overrides the band.  So if a band change is made after a mode
        # change it may make an incompatible pairing.
        self.add_item_to_command_list(self._set_mode, (mode, band), 1, 800)


    def _set_mode(self, mode, band=None):
        # Create the mode to popup item mapping
        mode_mapping = {ap_spec.MODE_B: '802.11b only',
            ap_spec.MODE_G: '802.11g only',
            ap_spec.MODE_N: '802.11n only',
            ap_spec.MODE_B | ap_spec.MODE_G: 'Mixed 802.11g and 802.11b',
            ap_spec.MODE_N | ap_spec.MODE_G: 'Mixed 802.11n and 802.11g',
            ap_spec.MODE_N | ap_spec.MODE_G | ap_spec.MODE_B:
            'Mixed 802.11n, 802.11g and 802.11b'}
        if mode in mode_mapping.keys():
            popup_value = mode_mapping[mode]
        else:
            raise SeleniumTimeoutException('The mode selected %s is not '
                                           'supported by router %s.' %
                                           (hex(mode), self.name))
        # When we change to an N based mode another popup is displayed.  We need
        # to wait for the before proceeding.
        wait_for_xpath = 'id("show_ssid")'
        if mode & ap_spec.MODE_N == ap_spec.MODE_N:
            wait_for_xpath = 'id("11n_protection")'
        self.select_item_from_popup_by_id(popup_value, 'dot11_mode',
                                          wait_for_xpath=wait_for_xpath)


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
        ssid = self.driver.find_element_by_id('show_ssid')
        checkbox = self.driver.find_element_by_id('w_enable')
        if ssid.get_attribute('disabled') == 'true':
            radio_enabled = False
        else:
            radio_enabled = True
        if radio_enabled == enabled:
            # Nothing to do
            return
        self.set_check_box_selected_by_id('w_enable', selected=False,
            wait_for_xpath='id("wep_type")')


    def set_ssid(self, ssid):
        # Can be done as long as it is enabled
        self.add_item_to_command_list(self._set_ssid, (ssid,), 1, 900)


    def _set_ssid(self, ssid):
        self._set_radio(enabled=True)
        self.set_content_of_text_field_by_id(ssid, 'show_ssid')
        self._ssid = ssid


    def set_channel(self, channel):
        self.add_item_to_command_list(self._set_channel, (channel,), 1, 900)


    def _set_channel(self, channel):
        position = self._get_channel_popup_position(channel)
        self._set_radio(enabled=True)
        channel_choices = ['2.412 GHz - CH 1 ', '2.417 GHz - CH 2',
                           '2.422 GHz - CH 3', '2.427 GHz - CH 4',
                           '2.432 GHz - CH 5', '2.437 GHz - CH 6',
                           '2.442 GHz - CH 7', '2.447 GHz - CH 8',
                           '2.452 GHz - CH 9', '2.457 GHz - CH 10',
                           '2.462 GHz - CH 11']
        channel_popup = self.driver.find_element_by_id('sel_wlan0_channel')
        if channel_popup.get_attribute('disabled') == 'true':
            self.set_check_box_selected_by_id('auto_channel', selected=False)
        self.select_item_from_popup_by_id(channel_choices[position],
                                          'sel_wlan0_channel')


    def set_band(self, band):
        logging.debug('This router (%s) does not support multiple bands.',
                      self.name)
        return None


    def set_security_disabled(self):
        self.add_item_to_command_list(self._set_security_disabled, (), 1, 900)


    def _set_security_disabled(self):
        self._set_radio(enabled=True)
        self.select_item_from_popup_by_id('None', 'wep_type')


    def set_security_wep(self, key_value, authentication):
        self.add_item_to_command_list(self._set_security_wep,
                                      (key_value, authentication), 1, 900)


    def _set_security_wep(self, key_value, authentication):
        self._set_radio(enabled=True)
        self.select_item_from_popup_by_id('WEP', 'wep_type',
                                          wait_for_xpath='id("key1")')
        self.select_item_from_popup_by_id(authentication, 'auth_type',
                                          wait_for_xpath='id("key1")')
        self.set_content_of_text_field_by_id(key_value, 'key1')


    def set_security_wpapsk(self, security, shared_key, update_interval=1800):
        self.add_item_to_command_list(self._set_security_wpapsk,
                                      (security, shared_key, update_interval),
                                       1, 900)


    def _set_security_wpapsk(self, security, shared_key, update_interval=1800):
        self._set_radio(enabled=True)
        self.select_item_from_popup_by_id('WPA-Personal', 'wep_type',
            wait_for_xpath='id("wlan0_gkey_rekey_time")')
        if security == ap_spec.SECURITY_TYPE_WPAPSK:
            wpa_item = 'WPA Only'
        else:
            wpa_item = 'WPA2 Only'
        self.select_item_from_popup_by_id(wpa_item, 'wpa_mode',
             wait_for_xpath='id("wlan0_psk_pass_phrase")')
        self.set_content_of_text_field_by_id(str(update_interval),
                                             'wlan0_gkey_rekey_time')
        self.set_content_of_text_field_by_id(shared_key,
                                             'wlan0_psk_pass_phrase')


    def set_visibility(self, visible=True):
        self.add_item_to_command_list(self._set_visibility, (visible,), 1, 900)


    def _set_visibility(self, visible=True):
        self._set_radio(enabled=True)
        # value=1 is visible; value=0 is invisible
        int_value = 1 if visible else 0
        xpath = ('//input[@value="%d" '
                 'and @name="wlan0_ssid_broadcast"]' % int_value)
        self.click_button_by_xpath(xpath, alert_handler=self._alert_handler)
