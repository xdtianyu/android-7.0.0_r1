# Copyright (c) 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import urlparse

import ap_spec
import dynamic_ap_configurator
from selenium.common.exceptions import NoSuchElementException as \
    SeleniumNoSuchElementException
from selenium.common.exceptions import WebDriverException
from selenium.common.exceptions import TimeoutException as \
    SeleniumTimeoutException


class BelkinF9K1102APConfigurator(
        dynamic_ap_configurator.DynamicAPConfigurator):
    """Base class for Belkin F9K1102 router."""


    def _security_alert(self, alert):
        text = alert.text
        if "It is recommended to use WPA/WPA2 when WPS is enabled" in text:
            alert.accept()
        elif "Selecting WEP Encryption will disable the WPS" in text:
            alert.accept()
        elif "Changing your security type will disable WPS" in text:
            alert.accept()
        elif 'Key0 is not complete' in text:
            raise RuntimeError('Got %s error. You should click the generate '
                               'button to generate a key first' % alert.text)
        else:
            raise RuntimeError('Unknown alert dialog' + alert.text)


    def get_supported_bands(self):
        return [{'band': ap_spec.BAND_2GHZ,
                 'channels': ['Auto', 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11]},
                {'band': ap_spec.BAND_5GHZ,
                 'channels': ['Auto', 36, 40, 44, 48, 149, 153, 157, 161, 165]}]


    def get_supported_modes(self):
        return [{'band': ap_spec.BAND_2GHZ,
                 'modes': [ap_spec.MODE_G,
                           ap_spec.MODE_B | ap_spec.MODE_G | ap_spec.MODE_N]},
                {'band': ap_spec.BAND_5GHZ,
                 'modes': [ap_spec.MODE_A, ap_spec.MODE_A | ap_spec.MODE_N]}]


    def get_number_of_pages(self):
        return 2


    def is_security_mode_supported(self, security_mode):
        """
        Returns if a given security_type is supported.

        @param security_mode: one security modes defined in the APSpec

        @return True if the security mode is supported; False otherwise.

        """
        return security_mode in (ap_spec.SECURITY_TYPE_DISABLED,
                                 ap_spec.SECURITY_TYPE_WPAPSK,
                                 ap_spec.SECURITY_TYPE_WEP)


    def navigate_to_page(self, page_number):
        """
        Navigates to the page corresponding to the given page number.

        This method performs the translation between a page number and a url to
        load. This is used internally by apply_settings.

        @param page_number: page number of the page to load

        """
        if page_number == 1:
            page_url = urlparse.urljoin(self.admin_interface_url,
                                        'wls_chan.html')
            self._load_the_page(page_url, page_title="Network Name")
        elif page_number == 2:
            page_url = urlparse.urljoin(self.admin_interface_url,
                                        'wls_sec.html')
            self._load_the_page(page_url, page_title="Security")
        else:
            raise RuntimeError('Invalid page number passed. Number of pages '
                               '%d, page value sent was %d' %
                               (self.get_number_of_pages(), page_number))


    def _load_the_page(self, page_url, page_title):
        """
        Load the given page and check if the title matches and we see the
        save_button object.

        @param page_url: The url of the page to load.
        @param page_title: The expected title of the page after it loads.

        """
        try:
            self.get_url(page_url, page_title)
        except:
            if 'Login' in self.driver.title:
                self._login(page_url)
        finally:
            try:
                self.wait_for_object_by_id('itsbutton1')
            except SeleniumTimeoutException, e:
                if 'Unable to find the object by xpath' in str(e):
                    xpath = "//h2[contains(.,'Duplicate Administrator')]"
                    if self.wait_for_object_by_xpath(xpath):
                        raise RuntimeError('We got a Duplicate Admin error.')
                else:
                    raise RuntimeError(str(e))


    def _login(self, page_url):
        """
        Login and wait for the object with obj_id to show up.

        @param page_url: The url of the page to load.

        """
        try:
            self.wait_for_object_by_id('p1210Password')
        except SeleniumNoSuchElementException, e:
            if (page_url in self.driver.current_url):
                logging.debug("In the login method, but we are already "
                              "logged in.")
            else:
                raise RuntimeError('We got a NoSuchElementException: ' + str(e))
        self.set_content_of_text_field_by_id('password', 'p1210Password',
                                             abort_check=True)
        self.click_button_by_id('p1210a005')
        self.wait_for_object_by_id('itsbutton1', wait_time=10)


    def save_page(self, page_number):
        """
        Saves the given page.

        @param page_number: Page number of the page to save.

        """
        button_id = 'itsbutton1'
        self.click_button_by_id(button_id, alert_handler=self._security_alert)
        self.set_wait_time(30)
        try:
            self.wait.until(lambda _:'Dashboard.htm' in self.driver.current_url)
        except SeleniumTimeoutException, e:
            if not (self.wait_for_object_by_id(button_id, wait_time=30)):
                raise RuntimeError('We did not save the page. '
                                   'We got a TimeoutException ' + str(e))
        finally:
            self.restore_default_wait_time()


    def set_radio(self, enabled=True):
        logging.debug('This router (%s) does not set the radio', self.name)
        return None


    def set_ssid(self, ssid):
        self.add_item_to_command_list(self._set_ssid, (ssid,), 1, 900)


    def _set_ssid(self, ssid):
        xpath = '//input[@name="wl_ssid"]'
        if self.current_band == ap_spec.BAND_5GHZ:
            xpath = '//input[@name="wl_ssid_5g"]'
        self.set_content_of_text_field_by_xpath(ssid, xpath, abort_check=False)
        self._ssid = ssid


    def set_channel(self, channel):
        self.add_item_to_command_list(self._set_channel, (channel,), 1, 900)


    def _set_channel(self, channel):
        position = self._get_channel_popup_position(channel)
        channel_choices = ['Auto', '1', '2', '3', '4', '5', '6', '7', '8',
                           '9', '10', '11']
        xpath = '//select[@name="wl_channel"]'
        if self.current_band == ap_spec.BAND_5GHZ:
            xpath = '//select[@name="wl_channel_5g"]'
            channel_choices = ['Auto', '36', '40', '44', '48', '149', '153',
                               '157', '161', '165']
        self.select_item_from_popup_by_xpath(channel_choices[position], xpath)


    def set_mode(self, mode, band=None):
        self.add_item_to_command_list(self._set_mode, (mode, band,), 1, 900)


    def _set_mode(self, mode, band=None):
        mode_mapping = {ap_spec.MODE_G: '802.11g', ap_spec.MODE_A: '802.11a',
                        ap_spec.MODE_N: '802.11n',
                        ap_spec.MODE_A | ap_spec.MODE_N: '802.11a & 802.11n',
                        ap_spec.MODE_B | ap_spec.MODE_G | ap_spec.MODE_N:
                        '802.11b & 802.11g & 802.11n'}
        mode_name = mode_mapping.get(mode)
        if not mode_name:
            raise RuntimeError('The mode %d not supported by router %s. ',
                               hex(mode), self.name)
        xpath = '//select[@name="wl_gmode"]'
        if self.current_band == ap_spec.BAND_5GHZ:
            xpath = '//select[@name="wl_gmode_5g"]'
        self.select_item_from_popup_by_xpath(mode_name, xpath)


    def set_band(self, band):
        if band == ap_spec.BAND_2GHZ:
            self.current_band = ap_spec.BAND_2GHZ
        elif band == ap_spec.BAND_5GHZ:
            self.current_band = ap_spec.BAND_5GHZ
        else:
            raise RuntimeError('Invalid band sent %s' % band)


    def _set_security(self, option, wait_for_xpath=None):
        popup = '//select[@name="security_mode"]'
        if self.current_band == ap_spec.BAND_5GHZ:
            popup = '//select[@name="security_mode_5g"]'
        try:
            self.select_item_from_popup_by_xpath(option, popup,
                                                 wait_for_xpath=wait_for_xpath,
                                                 alert_handler=
                                                 self._security_alert)
        except WebDriverException, e:
            message = str(e)
            if 'Selecting WEP Encryption will disable the WPS' in message:
                alert = self.driver.switch_to_alert()
                alert.accept()


    def set_security_disabled(self):
        self.add_item_to_command_list(self._set_security_disabled, (), 2, 1000)


    def _set_security_disabled(self):
        self._set_security('Off')


    def set_security_wep(self, key_value, authentication):
        self.add_item_to_command_list(self._set_security_wep,
                                      (key_value, authentication), 2, 1000)


    def _set_security_wep(self, key_value, authentication):
        text_field = '//input[@name="passphrase_64"]'
        generate_button = '//a[@id="wep64a_btn"]'
        key_field = '//input[@name="ENC11"]'
        if self.current_band == ap_spec.BAND_5GHZ:
            text_field = '//input[@name="passphrase_64_5g"]'
            generate_button = '//a[@id="wep64a_5_btn"]'
            key_field = '//input[@name="ENC511"]'
        self._set_security('64bit WEP', wait_for_xpath=text_field)
        self.set_content_of_text_field_by_xpath(key_value, text_field,
                                                abort_check=True)
        self.click_button_by_xpath(generate_button, alert_handler=
                                   self._security_alert)
        field = self.wait_for_object_by_xpath(key_field)
        self.wait.until(lambda _: field.get_attribute('value'))


    def set_security_wpapsk(self, security, shared_key, update_interval=None):
        self.add_item_to_command_list(self._set_security_wpapsk,
                                      (security, shared_key, update_interval),
                                       2, 900)


    def _set_security_wpapsk(self, security, shared_key, update_interval=None):
        auth_popup = '//select[@name="wl_sec_auth"]'
        psk_field = '//input[@name="wl_wpa2_psk2"]'
        if self.current_band == ap_spec.BAND_5GHZ:
            auth_popup = '//select[@name="wl_sec_auth_5g"]'
            psk_field = '//input[@name="wl_wpa2_psk2_5g"]'
        auth_type = 'WPA-PSK'
        if security == ap_spec.SECURITY_TYPE_WPA2PSK:
            auth_type = 'WPA2-PSK'
        self._set_security('WPA/WPA2-Personal(PSK)', wait_for_xpath=auth_popup)
        self.select_item_from_popup_by_xpath(auth_type, auth_popup,
                                             wait_for_xpath=psk_field,
                                             alert_handler=
                                             self._security_alert)
        self.set_content_of_text_field_by_xpath(shared_key, psk_field,
                                                abort_check=True)


    def is_visibility_supported(self):
        """
        Returns if AP supports setting the visibility (SSID broadcast).

        @return True if supported; False otherwise.
        """
        return False


    def is_update_interval_supported(self):
        """
        Returns True if setting the PSK refresh interval is supported.

        @return True is supported; False otherwise
        """
        return False
