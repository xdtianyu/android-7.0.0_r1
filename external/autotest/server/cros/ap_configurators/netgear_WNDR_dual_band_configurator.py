# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Base class for NetgearWNDR dual band routers."""

import urlparse
import dynamic_ap_configurator
import ap_spec

from selenium.common.exceptions import TimeoutException as \
    SeleniumTimeoutException


class NetgearDualBandAPConfigurator(
        dynamic_ap_configurator.DynamicAPConfigurator):
    """Base class for NetgearWNDR dual band routers."""


    def _alert_handler(self, alert):
        """Checks for any modal dialogs which popup to alert the user and
        either raises a RuntimeError or ignores the alert.

        Args:
          alert: The modal dialog's contents.
        """
        text = alert.text
        if 'WPA-PSK [TKIP] ONLY operates at \"Up to 54Mbps\"' in text:
            alert.accept()
            raise RuntimeError('Wrong mode selected. %s' % text)
        elif '2.4G and 5G have the same SSID' in text:
            alert.accept()
            raise RuntimeError('%s. Please change the SSID of one band' % text)
        elif 'do not want any wireless security on your network?' in text:
            alert.accept()
        elif 'recommends that you set the router to a high channel' in text:
            alert.accept()
        elif 'security authentication cannot work with WPS' in text:
            alert.accept()
        elif 'WPS requires SSID broadcasting in order to work' in text:
            alert.accept()
        else:
            raise RuntimeError('We have an unhandled alert on AP %s: %s' %
                               (self.host_name, text))


    def get_number_of_pages(self):
        return 1


    def is_update_interval_supported(self):
        """Returns True if setting the PSK refresh interval is supported.

        @return True is supported; False otherwise
        """
        return False


    def get_supported_bands(self):
        return [{'band': ap_spec.BAND_2GHZ,
                 'channels': ['Auto', 1, 2, 3, 4, 5, 6, 7, 8, 9 , 10, 11]},
                {'band': ap_spec.BAND_5GHZ,
                 'channels': ['Auto', 36, 40, 44, 48, 149, 153,
                              157, 161, 165]}]


    def get_supported_modes(self):
        return [{'band': ap_spec.BAND_2GHZ,
                 'modes': [ap_spec.MODE_G, ap_spec.MODE_N]},
                {'band': ap_spec.BAND_5GHZ,
                 'modes': [ap_spec.MODE_A, ap_spec.MODE_N]}]


    def is_security_mode_supported(self, security_mode):
        """Returns if the supported security modes.

        @param security_mode: the security mode to check against

        """
        return security_mode in (ap_spec.SECURITY_TYPE_DISABLED,
                                 ap_spec.SECURITY_TYPE_WPAPSK,
                                 ap_spec.SECURITY_TYPE_WPA2PSK,
                                 ap_spec.SECURITY_TYPE_WEP)


    def navigate_to_page(self, page_number):
        """Navigates to the given page.

        @param page_number: page number to open as an iteger

        """
        if page_number != 1:
            raise RuntimeError('Invalid page number passed.  Number of pages '
                               '%d, page value sent was %d' %
                               (self.get_number_of_pages(), page_number))
        page_url = urlparse.urljoin(self.admin_interface_url,
                                    'WLG_wireless_dual_band.htm')
        try:
            for i in range(5):
                self.get_url(page_url, page_title='NETGEAR Router')
                if 'NETGEAR Router' in self.driver.title:
                    break
        except SeleniumTimeoutException, e:
            xpath = '//button[@name="yes" and @class="purpleBtn"]'
            for i in range(5):
                element = self.wait_for_object_by_xpath(xpath)
                if element and element.is_displayed():
                    self.click_button_by_xpath(xpath)
                    break
            else:
                self.driver.refresh()
        self.wait_for_object_by_xpath('//input[@name="ssid" and @type="text"]')


    def save_page(self, page_number):
        """Saves all settings.

        @param page_number: the page to save.

        """
        self.click_button_by_xpath('//button[@name="Apply"]',
                                   alert_handler=self._alert_handler)


    def set_mode(self, mode, band=None):
        # The mode popup changes based on the security mode.  Set to no
        # security to get the right popup.
        self.add_item_to_command_list(self._set_security_disabled, (), 1, 600)
        self.add_item_to_command_list(self._set_mode, (mode, ), 1, 700)


    def _set_mode(self, mode, band=None):
        if mode == ap_spec.MODE_G or mode == ap_spec.MODE_A:
            mode = 'Up to 54 Mbps'
        elif mode == ap_spec.MODE_N:
            mode = 'Up to 300 Mbps'
        else:
            raise RuntimeError('Unsupported mode passed.')
        xpath = '//select[@name="opmode"]'
        if self.current_band == ap_spec.BAND_5GHZ:
            xpath = '//select[@name="opmode_an"]'
        self.wait_for_object_by_xpath(xpath)
        self.select_item_from_popup_by_xpath(mode, xpath,
                                             alert_handler=self._alert_handler)


    def set_radio(self, enabled=True):
        #  We cannot turn off the radio in Netgear
        return None


    def set_ssid(self, ssid):
        self.add_item_to_command_list(self._set_ssid, (ssid,), 1, 900)


    def _set_ssid(self, ssid):
        xpath = '//input[@name="ssid"]'
        if self.current_band == ap_spec.BAND_5GHZ:
            xpath = '//input[@name="ssid_an"]'
        self.set_content_of_text_field_by_xpath(ssid, xpath)
        self._ssid = ssid


    def set_channel(self, channel):
        self.add_item_to_command_list(self._set_channel, (channel,), 1, 800)


    def _set_channel(self, channel):
        position = self._get_channel_popup_position(channel)
        channel_choices = ['Auto', '01', '02', '03', '04', '05', '06', '07',
                           '08', '09', '10', '11']
        xpath = '//select[@name="w_channel"]'
        if self.current_band == ap_spec.BAND_5GHZ:
            xpath = '//select[@name="w_channel_an"]'
            channel_choices = ['Auto', '36', '40', '44', '48', '149', '153',
                               '157', '161', '165']
        self.select_item_from_popup_by_xpath(channel_choices[position],
                                             xpath,
                                             alert_handler=self._alert_handler)


    def set_band(self, band):
        if band == ap_spec.BAND_5GHZ:
            self.current_band = ap_spec.BAND_5GHZ
        elif band == ap_spec.BAND_2GHZ:
            self.current_band = ap_spec.BAND_2GHZ
        else:
            raise RuntimeError('Invalid band sent %s' % band)


    def set_security_disabled(self):
        self.add_item_to_command_list(self._set_security_disabled, (), 1, 1000)


    def _set_security_disabled(self):
        xpath = '//input[@name="security_type"]'
        if self.current_band == ap_spec.BAND_5GHZ:
            xpath = '//input[@name="security_type_an"]'
        self.click_button_by_xpath(xpath, alert_handler=self._alert_handler)


    def set_security_wep(self, key_value, authentication):
        # The button name seems to differ in various Netgear routers
        self.add_item_to_command_list(self._set_security_wep,
                                      (key_value, authentication), 1, 1000)


    def _set_security_wep(self, key_value, authentication):
        xpath = '//input[@name="security_type" and @value="WEP" and\
                 @type="radio"]'
        text_field = '//input[@name="passphraseStr"]'
        button = '//button[@name="keygen"]'
        if self.current_band == ap_spec.BAND_5GHZ:
            xpath = '//input[@name="security_type_an" and @value="WEP" and\
                     @type="radio"]'
            text_field = '//input[@name="passphraseStr_an"]'
            button = '//button[@name="Generate_an"]'
        try:
            self.wait_for_object_by_xpath(xpath)
            self.click_button_by_xpath(xpath, alert_handler=self._alert_handler)
        except Exception, e:
            raise RuntimeError('We got an exception: "%s". Check the mode. '
                               'It should be \'Up to 54 Mbps\'.' % str(e))
        self.wait_for_object_by_xpath(text_field)
        self.set_content_of_text_field_by_xpath(key_value, text_field,
                                                abort_check=True)
        self.click_button_by_xpath(button, alert_handler=self._alert_handler)


    def set_security_wpapsk(self, security, shared_key, update_interval=None):
        self.add_item_to_command_list(self._set_security_wpapsk,
                                      (security, shared_key,), 1, 1000)


    def _set_security_wpapsk(self, security, shared_key, update_interval=None):
        # Update Interval is not supported.
        if security == ap_spec.SECURITY_TYPE_WPAPSK:
            wpa_item = "WPA-PSK"
            # WPA-PSK is supported only in mode g and a (Up to 54 Mbps).
            # Setting correct mode before setting WPA-PSK.
            if self.current_band == ap_spec.BAND_2GHZ:
                self._set_mode(ap_spec.MODE_G, self.current_band)
            else:
                self._set_mode(ap_spec.MODE_A, self.current_band)
        else:
            wpa_item = "WPA2-PSK"
        xpath = ('//input[@name="security_type" and @value="%s"]' % wpa_item)
        text = '//input[@name="passphrase"]'
        if self.current_band == ap_spec.BAND_5GHZ:
            xpath = ('//input[@name="security_type_an" and @value="%s"]' %
                              wpa_item)
            text = '//input[@name="passphrase_an"]'
        try:
            self.click_button_by_xpath(xpath,
                                       alert_handler=self._alert_handler)
        except Exception, e:
            raise RuntimeError('For WPA-PSK the mode should be 54Mbps. %s' % e)
        self.set_content_of_text_field_by_xpath(shared_key, text,
                                                abort_check=True)


    def set_visibility(self, visible=True):
        # This router is very fussy with WPS even if it is not enabled.  It
        # throws an alert if visibility is off before you adjust security.
        # Bump visibilities priority to avoid that warning.
        self.add_item_to_command_list(self._set_visibility, (visible,), 1, 500)


    def _set_visibility(self, visible=True):
        xpath = '//input[@name="ssid_bc" and @type="checkbox"]'
        if self.current_band == ap_spec.BAND_5GHZ:
            xpath = '//input[@name="ssid_bc_an" and @type="checkbox"]'
        check_box = self.wait_for_object_by_xpath(xpath)
        # These check boxes behave different from other APs.
        value = check_box.is_selected()
        if (visible and not value) or (not visible and value):
            self.click_button_by_xpath(xpath, alert_handler=self._alert_handler)
