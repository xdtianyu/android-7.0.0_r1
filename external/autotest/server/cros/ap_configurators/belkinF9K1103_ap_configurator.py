# Copyright (c) 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import urlparse
import ap_spec
import dynamic_ap_configurator
from selenium.common.exceptions import WebDriverException
from selenium.common.exceptions import TimeoutException as \
    SeleniumTimeoutException


class BelkinF9K1103APConfigurator(
        dynamic_ap_configurator.DynamicAPConfigurator):
    """Class to configure Belkin F9K1103 v1 (01c) router."""


    def _security_alert(self, alert):
        text = alert.text
        if 'Invalid character' in text:
            alert.accept()
        elif 'It is recommended to use WPA/WPA2 when WPS is enabled' in text:
            alert.accept()
        else:
            alert.accept()
            raise RuntimeError('Unhandeled modal dialog. %s' % text)


    def _login(self):
        """Opens the login page and logs in using the password.
           We need to login before doing any other change to make sure that
           we have access to the router.
        """
        self.driver.delete_all_cookies()
        page_url = urlparse.urljoin(self.admin_interface_url,'login.htm')
        self.get_url(page_url)
        self.driver.switch_to_default_content()
        frame = self.driver.find_element_by_name('mainFrame')
        self.driver.switch_to_frame(frame)
        xpath = '//input[@name="ui_pws"]'
        try:
            self.set_content_of_text_field_by_xpath('password', xpath,
                                                    abort_check=True)
            self.click_button_by_id('412')
        except WebDriverException, e:
            element = self.driver.find_element_by_id('60')
            if 'Duplicate Administrator' in element.text:
                raise RuntimeError('Cannot login. Someone has already '
                                   'logged into the router. ' + str(e))
            else:
                raise WebDriverException('Cannot login. ' + str(e))


    def get_supported_bands(self):
        return [{'band': ap_spec.BAND_2GHZ,
                 'channels': ['Auto', 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11]},
                {'band': ap_spec.BAND_5GHZ,
                 'channels': ['Auto', 149, 153, 157, 161, 165]}]


    def get_supported_modes(self):
        return [{'band': ap_spec.BAND_2GHZ,
                 'modes': [ap_spec.MODE_G, ap_spec.MODE_N,
                           ap_spec.MODE_B | ap_spec.MODE_G | ap_spec.MODE_N]},
                {'band': ap_spec.BAND_5GHZ,
                 'modes': [ap_spec.MODE_A, ap_spec.MODE_N,
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
                                 ap_spec.SECURITY_TYPE_WEP)


    def navigate_to_page(self, page_number):
        """
        Navigates to the page corresponding to the given page number.

        This method performs the translation between a page number and a url to
        load. This is used internally by apply_settings.

        @param page_number: page number of the page to load

        """
        self._login()
        if page_number == 1:
            page_url = urlparse.urljoin(self.admin_interface_url, 'wifi_id.htm')
            self.get_url(page_url, page_title='Channel and SSID')
            self.driver.switch_to_default_content()
            frame = self.driver.find_element_by_name('mainFrame')
            self.driver.switch_to_frame(frame)
            self.wait_for_object_by_xpath('//input[@name="wifi_ssid"]')
        elif page_number == 2:
            page_url = urlparse.urljoin(self.admin_interface_url, 'wifi_e.htm')
            try:
                self.driver.get(page_url)
                self.wait.until(lambda _:'index.htm' in self.driver.title)
            except SeleniumTimeoutException, e:
                # The security page does not load properly, hence we
                # refresh if we don't get the intended page
                try:
                    self.driver.get(page_url)
                    self.wait.until(lambda _:'index.htm' in self.driver.title)
                except SeleniumTimeoutException, e:
                    raise SeleniumTimeoutException('Page did not load. '
                                                   + str(e))
            try:
                self.driver.switch_to_default_content()
                frame = self.driver.find_element_by_name('mainFrame')
                self.driver.switch_to_frame(frame)
                self.wait_for_object_by_xpath('//select[@name=wl_authmod]')
            except (WebDriverException, SeleniumTimeoutException), e:
                message = str(e)
                if (not any(alert in message for alert in [
                    'unexpected alert open', 'An open modal dialog blocked'])):
                    raise RuntimeError(message)
                    return
                self._security_alert(self.driver.switch_to_alert())
        else:
            raise RuntimeError('Invalid page number passed. Number of pages '
                               '%d, page value sent was %d' %
                               (self.get_number_of_pages(), page_number))


    def save_page(self, page_number):
        """Save changes and logout from the router.

        @param page_number: the page number to save as an integer.

        """
        self.click_button_by_xpath('//input[@type="button" and '
                                   '@value="Apply Changes"]',
                                   alert_handler=self._security_alert)
        self.set_wait_time(120)
        try:
            self.wait.until(lambda _:'Status' in self.driver.title)
        except SeleniumTimeoutException, e:
            try:
                # The webpages of this router have a tendency to not load
                # completely, hence we refresh before we raise an excpetion.
                self.driver.refresh()
                self.wait.until(lambda _:'Status' in self.driver.title)
            except SeleniumTimeoutException, e:
                raise SeleniumTimeoutException('The changes were not saved. '
                                               '%s' % str(e))
        finally:
            self.restore_default_wait_time()


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
            channel_choices = ['Auto', '149', '153', '157', '161', '165']
        self.select_item_from_popup_by_xpath(channel_choices[position], xpath)


    def set_mode(self, mode):
        self.add_item_to_command_list(self._set_mode, (mode,), 1, 900)


    def _set_mode(self, mode):
        mode_mapping = {ap_spec.MODE_G: '802.11g', ap_spec.MODE_A: '802.11a',
                        ap_spec.MODE_N: '802.11n',
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
        self.select_item_from_popup_by_xpath(mode_name, xpath,
                                             wait_for_xpath=None,
                                             alert_handler=self._security_alert)


    def set_radio(self, enabled=True):
        logging.debug('This router (%s) does not support radio',
                      self.name)
        return None


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
        generate_button = '//input[@id="811" and type="button" and \
                          @onclick="Gen64bitkey(0)"]'
        if self.current_band == ap_spec.BAND_5GHZ:
            text_field = '//input[@name="wl1_phrase"]'
            generate_button = '//input[@id="811" and type="button" and \
                               @onclick="Gen64bitkey(1)"]'
        self._set_security('64bit WEP', wait_for_xpath=text_field)
        self.set_content_of_text_field_by_xpath(key_value, text_field,
                                                abort_check=True)
        self.click_button_by_id(generate_button)


    def set_security_wpapsk(self, shared_key, update_interval=None):
        self.add_item_to_command_list(self._set_security_wpapsk,
                                      (shared_key, update_interval), 2, 900)


    def _set_security_wpapsk(self, shared_key, update_interval=None):
        auth_popup = '//select[@name="wl_auth"]'
        psk_field = '//input[@name="wl_wpa_ks_txt"]'
        if self.current_band == ap_spec.BAND_5GHZ:
            auth_popup = '//select[@name="wl1_auth"]'
            psk_field = '//input[@name="wl1_wpa_ks_txt"]'
        self._set_security('WPA/WPA2-Personal (PSK)', wait_for_xpath=auth_popup)
        self.select_item_from_popup_by_xpath('WPA-PSK', auth_popup,
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
