# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Class to control the DlinkAP router."""

import os

import dynamic_ap_configurator
import ap_spec
from selenium.common.exceptions import TimeoutException as \
    SeleniumTimeoutException

class DLinkAPConfigurator(
        dynamic_ap_configurator.DynamicAPConfigurator):
    """Derived class to control the DLink DAP-1522."""

    def _open_landing_page(self):
        self.get_url('%s/index.php' % self.admin_interface_url,
                     page_title='D-Link Corporation')
        page_name = os.path.basename(self.driver.current_url)
        if page_name == 'login.php' or page_name == 'index.php':
            try:
                self.wait_for_object_by_xpath('//*[@name="login"]')
            except SeleniumTimeoutException, e:
                # Maybe we were re-routed to the configuration page
                if (os.path.basename(self.driver.current_url) ==
                    'bsc_wizard.php'):
                    return
                raise SeleniumTimeoutException('Unable to navigate to the '
                                               'login or configuration page. '
                                               'WebDriver exception:%s', str(e))
            login_button = self.driver.find_element_by_xpath(
                '//*[@name="login"]')
            login_button.click()


    def _open_configuration_page(self):
        self._open_landing_page()
        if os.path.basename(self.driver.current_url) != 'bsc_wizard.php':
            raise SeleniumTimeoutException('Taken to an unknown page %s' %
                os.path.basename(self.driver.current_url))

        # Else we are being logged in automatically to the landing page
        wlan = '//*[@name="wlan_wireless"]'
        self.wait_for_object_by_xpath(wlan)
        wlan_button = self.driver.find_element_by_xpath(wlan)
        wlan_button.click()
        # Wait for the main configuration page, look for the radio button
        self.wait_for_object_by_id('enable')


    def get_number_of_pages(self):
        return 1


    def get_supported_bands(self):
        return [{'band': ap_spec.BAND_2GHZ,
                 'channels': [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11]},
                {'band': ap_spec.BAND_5GHZ,
                 'channels': [26, 40, 44, 48, 149, 153, 157, 161, 165]}]


    def get_supported_modes(self):
        return [{'band': ap_spec.BAND_2GHZ,
                 'modes': [ap_spec.MODE_B, ap_spec.MODE_G, ap_spec.MODE_N,
                           ap_spec.MODE_B | ap_spec.MODE_G,
                           ap_spec.MODE_G | ap_spec.MODE_N]},
                {'band': ap_spec.BAND_5GHZ,
                 'modes': [ap_spec.MODE_A, ap_spec.MODE_N,
                           ap_spec.MODE_A | ap_spec.MODE_N]}]


    def is_security_mode_supported(self, security_mode):
        """
        Returns if a given security_type is supported.

        @param security_mode: one security modes defined in the APSpec

        @return True if the security mode is supported; False otherwise.

        """
        return security_mode in (self.security_disabled,
                                 self.security_wpapsk,
                                 self.security_wep)


    def navigate_to_page(self, page_number):
        """
        Navigates to the page corresponding to the given page number.

        This method performs the translation between a page number and a url to
        load. This is used internally by apply_settings.

        @param page_number: page number of the page to load

        """
        # All settings are on the same page, so we always open the config page
        self._open_configuration_page()


    def save_page(self, page_number):
        """
        Saves the given page.

        @param page_number: Page number of the page to save.

        """
        # All settings are on the same page, we can ignore page_number
        button = self.driver.find_element_by_xpath('//input[@name="apply"]')
        button.click()
        # If we did not make changes we are sent to the continue screen.
        continue_screen = True
        button_xpath = '//input[@name="bt"]'
        try:
            self.wait_for_object_by_xpath(button_xpath)
        except SeleniumTimeoutException, e:
            continue_screen = False
        if continue_screen:
            button = self.driver.find_element_by_xpath(button_xpath)
            button.click()
        # We will be returned to the landing page when complete
        self.wait_for_object_by_id('enable')


    def set_mode(self, mode, band=None):
        # Mode overrides the band.  So if a band change is made after a mode
        # change it may make an incompatible pairing.
        self.add_item_to_command_list(self._set_mode, (mode, band), 1, 800)


    def _set_mode(self, mode, band=None):
        # Create the mode to popup item mapping
        mode_mapping = {ap_spec.MODE_B: '802.11b Only',
            ap_spec.MODE_G: '802.11g Only',
            ap_spec.MODE_N: '802.11n Only',
            ap_spec.MODE_B | ap_spec.MODE_G: 'Mixed 802.11g and 802.11b',
            ap_spec.MODE_N | ap_spec.MODE_G: 'Mixed 802.11n and 802.11g',
            ap_spec.MODE_N | ap_spec.MODE_G | ap_spec.MODE_B:
            'Mixed 802.11n, 802.11g, and 802.11b',
            ap_spec.MODE_N | ap_spec.MODE_G | ap_spec.MODE_B:
            'Mixed 802.11n, 802.11g, and 802.11b',
            ap_spec.MODE_A: '802.11a Only',
            ap_spec.MODE_N | ap_spec.MODE_A: 'Mixed 802.11n and 802.11a'}
        band_value = ap_spec.BAND_2GHZ
        if mode in mode_mapping.keys():
            popup_value = mode_mapping[mode]
            # If the mode contains 802.11a we use 5Ghz
            if mode & ap_spec.MODE_A == ap_spec.MODE_A:
                band_value = ap_spec.BAND_5GHZ
            # If the mode is 802.11n mixed with 802.11a it must be 5Ghz
            elif (mode & (ap_spec.MODE_N | ap_spec.MODE_A) ==
                 (ap_spec.MODE_N | ap_spec.MODE_A)):
                band_value = ap_spec.BAND_5GHZ
            # If the mode is 802.11n mixed with other than 802.11a its 2Ghz
            elif (mode & ap_spec.MODE_N == ap_spec.MODE_N and
                  mode ^ ap_spec.MODE_N > 0):
                band_value = ap_spec.BAND_2GHZ
            # If the mode is 802.11n then default to 5Ghz unless there is a band
            elif mode == ap_spec.MODE_N:
                band_value = ap_spec.BAND_5GHZ
            if band:
                band_value = band
        else:
            raise SeleniumTimeoutException('The mode selected %s is not '
                                           'supported by router %s.' %
                                           (hex(mode), self.name))
        # Set the band first
        self._set_band(band_value)
        popup_id = 'mode_80211_11g'
        if band_value == ap_spec.BAND_5GHZ:
            popup_id = 'mode_80211_11a'
        self.select_item_from_popup_by_id(popup_value, popup_id)


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
        ssid = self.driver.find_element_by_xpath('//input[@name="ssid"]')
        if ssid.get_attribute('disabled') == 'true':
            radio_enabled = False
        else:
            radio_enabled = True
        if radio_enabled == enabled:
            # Nothing to do
            return
        self.set_check_box_selected_by_id('enable', selected=False,
            wait_for_xpath='id("security_type_ap")')


    def set_ssid(self, ssid):
        # Can be done as long as it is enabled
        self.add_item_to_command_list(self._set_ssid, (ssid,), 1, 900)


    def _set_ssid(self, ssid):
        self._set_radio(enabled=True)
        self.set_content_of_text_field_by_id(ssid, 'ssid')
        self._ssid = ssid


    def set_channel(self, channel):
        self.add_item_to_command_list(self._set_channel, (channel,), 1, 900)


    def _set_channel(self, channel):
        position = self._get_channel_popup_position(channel)
        self._set_radio(enabled=True)
        self.set_check_box_selected_by_id('autochann', selected=False)
        self.select_item_from_popup_by_id(str(position), 'channel_g')


    def get_band(self):
        """
        This is experimental
        The radio buttons do more than run a script that adjusts the possible
        channels. We will just check the channel to popup.
        """
        self.set_radioSetting(enabled=True)
        xpath = ('id("channel_g")')
        self._open_configuration_page()
        self.wait_for_object_by_xpath(xpath)
        element = self.driver.find_element_by_xpath(xpath)
        if element.find_elements_by_tag_name('option')[0].text == '1':
            return ap_spec.BAND_2GHZ
        return ap_spec.BAND_5GHZ


    def set_band(self, band):
        if band != ap_spec.BAND_2GHZ or band != ap_spec.BAND_5GHZ:
            raise RuntimeError('Invalid band sent %s' % band)
        self.add_item_to_command_list(self._set_band, (band,), 1, 900)


    def _set_band(self, band):
        self._set_radio(enabled=True)
        if band == ap_spec.BAND_2GHZ:
            int_value = 0
            wait_for_id = 'mode_80211_11g'
        elif band == ap_spec.BAND_5GHZ:
            int_value = 1
            wait_for_id = 'mode_80211_11a'
            xpath = ('//*[contains(@class, "l_tb")]/input[@value="%d" '
                     'and @name="band"]' % int_value)
            element = self.driver.find_element_by_xpath(xpath)
            element.click()
        self.wait_for_object_by_id(wait_for_id)


    def set_security_disabled(self):
        self.add_item_to_command_list(self._set_security_disabled, (), 1, 900)


    def _set_security_disabled(self):
        self._set_radio(enabled=True)
        security_disabled = 'Disable Wireless Security (not recommended)'
        self.select_item_from_popup_by_id(security_disabled, 'security_type_ap')


    def set_security_wep(self, key_value, authentication):
        self.add_item_to_command_list(self._set_security_wep,
                                      (key_value, authentication), 1, 900)


    def _set_security_wep(self, key_value, authentication):
        self._set_radio(enabled=True)
        self.select_item_from_popup_by_id('WEP', 'security_type_ap',
                                          wait_for_xpath='id("auth_type")')
        self.select_item_from_popup_by_id(authentication, 'auth_type',
                                          wait_for_xpath='id("wep_key_value")')
        self.set_content_of_text_field_by_id(key_value, 'wep_key_value')
        self.set_content_of_text_field_by_id(key_value, 'verify_wep_key_value')


    def set_security_wpapsk(self, shared_key, update_interval=1800):
        self.add_item_to_command_list(self._set_security_wpapsk,
                                      (shared_key, update_interval), 1, 900)


    def _set_security_wpapsk(self, shared_key, update_interval=1800):
        self._set_radio(enabled=True)
        self.select_item_from_popup_by_id('WPA-Personal',
                                          'security_type_ap',
                                          wait_for_xpath='id("wpa_mode")')
        self.select_item_from_popup_by_id('WPA Only', 'wpa_mode',
            wait_for_xpath='id("grp_key_interval")')
        self.set_content_of_text_field_by_id(str(update_interval),
                                             'grp_key_interval')
        self.set_content_of_text_field_by_id(shared_key, 'wpapsk1')


    def set_visibility(self, visible=True):
        self.add_item_to_command_list(self._set_visibility, (visible,), 1, 900)


    def _set_visibility(self, visible=True):
        self._set_radio(enabled=True)
        # value=0 is visible; value=1 is invisible
        int_value = int(not visible)
        xpath = ('//*[contains(@class, "l_tb")]/input[@value="%d" '
                 'and @name="visibility_status"]' % int_value)
        self.click_button_by_xpath(xpath)
