# Copyright (c) 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import urlparse

import dynamic_ap_configurator
import ap_spec
from selenium.common.exceptions import NoSuchElementException as \
    SeleniumNoSuchElementException
from selenium.common.exceptions import WebDriverException
from selenium.common.exceptions import TimeoutException


class BelkinF9K1105APConfigurator(
        dynamic_ap_configurator.DynamicAPConfigurator):
    """Base class for Belkin F9K1105 router."""


    def _security_alert(self, alert):
        text = alert.text
        if "It is recommended to use WPA/WPA2 when WPS is enabled" in text:
            alert.accept()
        elif "Selecting WEP Encryption will disable the WPS" in text:
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
                 'channels': ['Auto', 36, 40, 44, 48, 149, 153, 157, 161]}]


    def get_supported_modes(self):
        return [{'band': ap_spec.BAND_2GHZ,
                 'modes': [ap_spec.MODE_G, ap_spec.MODE_N,
                           ap_spec.MODE_B | ap_spec.MODE_G | ap_spec.MODE_N]},
                {'band': ap_spec.BAND_5GHZ,
                 'modes': [ap_spec.MODE_N, ap_spec.MODE_A,
                           ap_spec.MODE_A | ap_spec.MODE_N]}]


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
                                 ap_spec.SECURITY_TYPE_WPA2PSK,
                                 ap_spec.SECURITY_TYPE_WEP)


    def navigate_to_page(self, page_number):
        """
        Navigates to the page corresponding to the given page number.

        This method performs the translation between a page number and a url to
        load. This is used internally by apply_settings.

        @param page_number: page number of the page to load

        """
        if page_number == 1:
            page_url = urlparse.urljoin(self.admin_interface_url, 'wifi_id.htm')
            self._load_the_page(page_url, page_title="Network Name")
        elif page_number == 2:
            page_url = urlparse.urljoin(self.admin_interface_url, 'wifi_sc.htm')
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
        @param page_title: The title of the page we are loading.

        """
        self.set_wait_time(20)  # The webpage takes a long time to load.
        try:
            self.get_url(page_url)
            self.wait.until(lambda _:page_title in self.driver.title)
            if 'dashboard' in self.driver.title:
                # This is a workaround for an issue where the wait would return
                # even though the page title is not what we expect.
                self._login(page_url, page_title)
        except TimeoutException, e:
            dup = '//h1[contains(text(), "Duplicate Administrator")]'
            if self.driver.find_element_by_id('p1210a005'):
                self._login(page_url, page_title)
            elif self.driver.find_element_by_xpath(dup).is_displayed():
                raise RuntimeError('We got the Duplicate admin message. '
                                   'Some one has already logged into the '
                                   'router. So we cannot login.')
        finally:
            self.set_wait_time(20)
            self.wait.until(lambda _:page_title in self.driver.title)
            self.restore_default_wait_time()


    def _login(self, page_url, page_title):
        """
        Login to the router.

        @param page_url: The url of the page to load.
        @param page_title: The title of the page we are loading.

        """
        try:
            self.wait_for_object_by_id('p1210Password')
        except SeleniumNoSuchElementException, e:
            if page_url in self.driver.current_url():
                logging.debug("In the login method, but we are already "
                              "logged in.")
            else:
                raise RuntimeError('We could not load the page ' + page_url +
                                   str(e))
        self.set_content_of_text_field_by_id('password', 'p1210Password',
                                                abort_check=True)
        self.click_button_by_id('p1210a005')
        pwd_wrong = '//small[@class="error" and @id="errpwderr"]'
        if self.driver.find_element_by_xpath(pwd_wrong).is_displayed():
            try:
                self.wait.until(lambda _:page_title in self.driver.title)
            except TimeoutException, e:
                raise RuntimeError('Incorrect password error: '
                                   'The router is not accepting the password.')


    def save_page(self, page_number):
        """
        Saves the given page.

        @param page_number: Page number of the page to save.

        """
        if page_number == 1:
            button_id = 'dnsapply'
        elif page_number == 2:
            button_id = 'btnapply'
        if self.driver.find_element_by_id(button_id).is_displayed():
            self.click_button_by_id(button_id)
            page_title = 'Welcome to your Belkin router dashboard!'
            # The page reloads in about 80 secs and goes back to the dashboard.
            # The device reboots to apply the changes, hence this delay.
            try:
                self.set_wait_time(120)
                self.wait.until(lambda _: page_title in self.driver.title)
            except:
                self.driver.refresh()
                # If page did not load even after a refresh just continue
                # because we already clicked the save button.
                if not page_title in self.driver.title:
                    pass
            finally:
                self.restore_default_wait_time()
        else:
            raise RuntimeError("We did not save the changes because we "
                               "could not find the button.")


    def set_radio(self, enabled=True):
        logging.debug('This router (%s) does not set the radio',
                      self.name)
        return None


    def set_ssid(self, ssid):
        self.add_item_to_command_list(self._set_ssid, (ssid,), 1, 900)


    def _set_ssid(self, ssid):
        xpath = '//input[@name="wifi_ssid"]'
        if self.current_band == ap_spec.BAND_5GHZ:
            xpath = '//input[@name="wifi_ssid1"]'
        self.set_content_of_text_field_by_xpath(ssid, xpath, abort_check=False)
        self._ssid = ssid


    def set_channel(self, channel):
        self.add_item_to_command_list(self._set_channel, (channel,), 1, 900)


    def _set_channel(self, channel):
        position = self._get_channel_popup_position(channel)
        channel_choices = ['Auto', '1', '2', '3', '4', '5', '6', '7', '8',
                           '9', '10', '11']
        xpath = '//select[@name="wchan"]'
        if self.current_band == ap_spec.BAND_5GHZ:
            xpath = '//select[@name="wchan1"]'
            channel_choices = ['Auto', '36', '40', '44', '48', '149', '153',
                               '157', '161']
        self.select_item_from_popup_by_xpath(channel_choices[position], xpath)


    def set_mode(self, mode, band=None):
        self.add_item_to_command_list(self._set_mode, (mode, band,), 1, 900)


    def _set_mode(self, mode, band=None):
        mode_mapping = {ap_spec.MODE_G: '802.11 g', ap_spec.MODE_A: '802.11 a',
                        ap_spec.MODE_N: '802.11 n',
                        ap_spec.MODE_A | ap_spec.MODE_N: '802.11a & 802.11n',
                        ap_spec.MODE_B | ap_spec.MODE_G | ap_spec.MODE_N:
                        '802.11b & 802.11g & 802.11n'}
        mode_name = mode_mapping.get(mode)
        if not mode_name:
            raise RuntimeError('The mode %d not supported by router %s. ',
                               hex(mode), self.name)
        xpath = '//select[@name="wbr"]'
        if self.current_band == ap_spec.BAND_5GHZ:
            xpath = '//select[@name="wbr1"]'
        self.select_item_from_popup_by_xpath(mode_name, xpath)


    def set_band(self, band):
        if band == ap_spec.BAND_2GHZ:
            self.current_band = ap_spec.BAND_2GHZ
        elif band == ap_spec.BAND_5GHZ:
            self.current_band = ap_spec.BAND_5GHZ
        else:
            raise RuntimeError('Invalid band sent %s' % band)


    def _set_security(self, option, wait_for_xpath=None):
        popup = '//select[@name="wl_authmod"]'
        if self.current_band == ap_spec.BAND_5GHZ:
            popup = '//select[@name="wl_authmod1"]'
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
        self._set_security('Disabled')


    def set_security_wep(self, key_value, authentication):
        self.add_item_to_command_list(self._set_security_wep,
                                      (key_value, authentication), 2, 1000)


    def _set_security_wep(self, key_value, authentication):
        text_field = '//input[@name="wl_phrase"]'
        generate_button = '//a[@id="btngen" and \
                           @onclick="return Gen64bitkey(0)"]'
        if self.current_band == ap_spec.BAND_5GHZ:
            text_field = '//input[@name="wl1_phrase"]'
            generate_button = '//a[@id="btngen" and \
                               @onclick="return Gen64bitkey(1)"]'
        self._set_security('64bit WEP', wait_for_xpath=text_field)
        self.set_content_of_text_field_by_xpath(key_value, text_field,
                                                abort_check=True)
        self.click_button_by_xpath(generate_button)


    def set_security_wpapsk(self, security, shared_key, update_interval=None):
        self.add_item_to_command_list(self._set_security_wpapsk,
                                      (security, shared_key, update_interval),
                                      2, 900)


    def _set_security_wpapsk(self, security, shared_key, update_interval=None):
        auth_popup = '//select[@name="wl_auth"]'
        psk_field = '//input[@name="wl_wpa_ks_pwd"]'
        if self.current_band == ap_spec.BAND_5GHZ:
            auth_popup = '//select[@name="wl1_auth"]'
            psk_field = '//input[@name="wl1_wpa_ks_pwd"]'
        self._set_security('WPA/WPA2-Personal (PSK)', wait_for_xpath=auth_popup)
        selection = 'WPA2-PSK'
        if security == ap_spec.SECURITY_TYPE_WPAPSK:
            selection = 'WPA-PSK'
        self.select_item_from_popup_by_xpath(selection, auth_popup,
                                             wait_for_xpath=psk_field,
                                             alert_handler=self._security_alert)
        self.set_content_of_text_field_by_xpath(shared_key, psk_field,
                                                abort_check=False)


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
