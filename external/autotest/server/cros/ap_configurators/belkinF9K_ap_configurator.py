# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Class to control the Belkin F9K router."""

import logging
import urlparse
import time
import dynamic_ap_configurator
import ap_spec
from selenium.webdriver.common.keys import Keys
from selenium.common.exceptions import WebDriverException
from selenium.common.exceptions import TimeoutException as \
    SeleniumTimeoutException


class BelkinF9KAPConfigurator(
        dynamic_ap_configurator.DynamicAPConfigurator):
    """Class to configure Blekin f9k1002v4 router."""


    def __init__(self, ap_config):
        super(BelkinF9KAPConfigurator, self).__init__(ap_config)
        self._dhcp_delay = 30


    security_popup = '//select[@name="security_type"]'

    def _security_alert(self, alert):
        text = alert.text
        if 'Invalid character' in text:
            alert.accept()
        elif 'It is recommended to use WPA/WPA2 when WPS is enabled' in text:
            alert.accept()
        elif 'After changing to 11g mode' in text:
            alert.accept()
        elif 'After changing to 11b&g&n or 11n mode' in text:
            alert.accept()
        else:
            alert.accept()
            raise RuntimeError('Unhandeled modal dialog. %s' % text)


    def open_new_tab(self):
         """Re-Opens tab on the browser"""
         body = self.driver.find_element_by_tag_name("body")
         body.send_keys(Keys.CONTROL + 't')


    def _login(self):
        """Opens the login page and logs in using the password.
           We need to login before doing any other change to make sure that
           we have access to the router."""
        page_url = urlparse.urljoin(self.admin_interface_url,'login.stm')
        xpath = '//input[@name="pws"]'
        try:
            self.open_new_tab()
            self.get_url(page_url, page_title='login.stm')
            self.wait_for_object_by_xpath(xpath, wait_time=10)
        except WebDriverException as e:
            logging.info("Page did not load or %s", str(e))
            self.driver.refresh()
        self.wait_for_object_by_xpath(xpath, wait_time=10)
        self.set_content_of_text_field_by_xpath('password', xpath,
                                                abort_check=True)
        self.click_button_by_xpath('//input[@value="Submit"]',
                                   alert_handler=self._security_alert)


    def get_supported_bands(self):
        return [{'band': ap_spec.BAND_2GHZ,
                 'channels': ['Auto', 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11]}]


    def get_supported_modes(self):
        return [{'band': ap_spec.BAND_2GHZ,
                 'modes': [ap_spec.MODE_G, ap_spec.MODE_N,
                           ap_spec.MODE_B | ap_spec.MODE_G | ap_spec.MODE_N]}]


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
        self._login()
        if page_number == 1:
            page_url = urlparse.urljoin(self.admin_interface_url,
                                        'wireless_id.stm')
            self.get_url(page_url, page_title='wireless_id')
            self.wait_for_object_by_xpath('//input[@name="ssid"]')
        elif page_number == 2:
            page_url = urlparse.urljoin(self.admin_interface_url,
                                        'wireless_e.stm')
            try:
                self.get_url(page_url, page_title='wireless')
                self.wait_for_object_by_xpath(self.security_popup)
            except (WebDriverException, SeleniumTimeoutException), e:
                message = str(e)
                if 'Timed out receiving message from renderer' in message:
                    self.driver.refresh()
                    self.wait_for_object_by_xpath(self.security_popup)
                elif (any(alert in message for alert in
                    ['unexpected alert open', 'An open modal dialog blocked'])):
                    self._security_alert(self.driver.switch_to_alert())
                else:
                    raise RuntimeError(message)
        else:
            raise RuntimeError('Invalid page number passed. Number of pages '
                               '%d, page value sent was %d' %
                               (self.get_number_of_pages(), page_number))


    def save_page(self, page_number):
        """Save changes and logout from the router.

        @param page_number: the page number to save as an integer.

        """
        self.click_button_by_xpath('//input[@type="submit" and '
                                   '@value="Apply Changes"]',
                                   alert_handler=self._security_alert)
        try:
            self.wait_for_object_by_xpath_to_vanish('//input[@name= \
                                                    "timeRemain]"',wait_time=30)
            self.wait.until(lambda _:'setup.htm' in self.driver.title)
        except WebDriverException, e:
            logging.info("Driver title page did not load or %s", str(e))
            self.open_new_tab()
        finally:
            self.restore_default_wait_time()


    def set_ssid(self, ssid):
        self.add_item_to_command_list(self._set_ssid, (ssid,), 1, 900)


    def _set_ssid(self, ssid):
        xpath = '//input[@name="ssid"]'
        self.set_content_of_text_field_by_xpath(ssid, xpath, abort_check=True)
        self._ssid = ssid


    def set_channel(self, channel):
        self.add_item_to_command_list(self._set_channel, (channel,), 1, 900)


    def _set_channel(self, channel):
        position = self._get_channel_popup_position(channel)
        channel_choices = ['Auto', '1', '2', '3', '4', '5', '6', '7', '8',
                           '9', '10', '11']
        xpath = '//select[@name="wchan"]'
        self.select_item_from_popup_by_xpath(channel_choices[position], xpath)


    def set_mode(self, mode):
        self.add_item_to_command_list(self._set_mode, (mode,), 1, 900)


    def _set_mode(self, mode):
        mode_mapping = {ap_spec.MODE_G: '802.11g',
                        ap_spec.MODE_N: '802.11n',
                        ap_spec.MODE_B | ap_spec.MODE_G | ap_spec.MODE_N:
                        '802.11b&802.11g&802.11n'}
        mode_name = mode_mapping.get(mode)
        if not mode_name:
            raise RuntimeError('The mode %d not supported by router %s. ',
                               hex(mode), self.name)
        xpath = '//select[@name="wbr"]'
        self.wait_for_object_by_xpath(xpath)
        while self.number_of_items_in_popup_by_xpath(xpath) < 3:
            time.sleep(0.25)
        self.select_item_from_popup_by_xpath(mode_name, xpath,
                                             wait_for_xpath=None,
                                             alert_handler=self._security_alert)


    def set_ch_width(self, channel_width):
        """
        Adjusts the channel width.

        @param channel_width: the channel width
        """
        self.add_item_to_command_list(self._set_ch_width,(channel_width,),
                                      1, 900)


    def _set_ch_width(self, channel_width):
        channel_choice = ['20MHz', '20/40MHz']
        xpath = '//select[@name="bandwidth"]'
        self.select_item_from_popup_by_xpath(channel_choice[channel_width],
                                             xpath)


    def set_radio(self, enabled=True):
        logging.debug('This router (%s) does not support radio', self.name)
        return None


    def set_band(self, band):
        logging.debug('This router %s does not support multiple bands.',
                      self.name)
        return None


    def set_security_disabled(self):
        self.add_item_to_command_list(self._set_security_disabled, (), 2, 1000)


    def _set_security_disabled(self):
        self.select_item_from_popup_by_xpath('Disabled',
                                             self.security_popup,
                                             alert_handler=self._security_alert)


    def set_security_wep(self, key_value, authentication):
        self.add_item_to_command_list(self._set_security_wep,
                                      (key_value, authentication), 2, 1000)


    def _set_security_wep(self, key_value, authentication):
        text_field = '//input[@name="passphrase"]'
        try:
            self.select_item_from_popup_by_xpath('64bit WEP',
                    self.security_popup, wait_for_xpath=text_field,
                    alert_handler=self._security_alert)
        except WebDriverException, e:
            message = str(e)
            if message.find('An open modal dialog blocked') == -1:
               raise RuntimeError(message)
               return
            self._security_alert(self.driver.switch_to_alert())
        self.set_content_of_text_field_by_xpath(key_value, text_field,
                                                abort_check=True)
        self.click_button_by_xpath('//input[@class="submitBtn" and '
                                   '@value="Generate"]',
                                   alert_handler=self._security_alert)


    def set_security_wpapsk(self, security, shared_key, update_interval=None):
        self.add_item_to_command_list(self._set_security_wpapsk,
                                      (security, shared_key, update_interval),
                                       2, 900)


    def _set_security_wpapsk(self, security, shared_key, update_interval=None):
        key_field = '//input[@name="wpa_key_pass"]'
        psk = '//select[@name="authentication"]'
        self.select_item_from_popup_by_xpath('WPA/WPA2-Personal (PSK)',
                                             self.security_popup,
                                             wait_for_xpath=key_field,
                                             alert_handler=self._security_alert)
        auth_type = 'WPA-PSK'
        if security == ap_spec.SECURITY_TYPE_WPA2PSK:
            auth_type = 'WPA2-PSK'
        self.select_item_from_popup_by_xpath(auth_type, psk,
                                             wait_for_xpath=key_field,
                                             alert_handler=self._security_alert)
        self.set_content_of_text_field_by_xpath(shared_key, key_field,
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
