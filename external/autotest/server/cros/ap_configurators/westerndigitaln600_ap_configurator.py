# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import urlparse
import time

import dynamic_ap_configurator
import ap_spec

from selenium.common.exceptions import TimeoutException
from selenium.common.exceptions import WebDriverException
from selenium.webdriver.support import expected_conditions as ec
from selenium.webdriver.common.by import By

class WesternDigitalN600APConfigurator(
        dynamic_ap_configurator.DynamicAPConfigurator):
    """Base class for objects to configure Western Digital N600 access point
       using webdriver."""


    def _sec_alert(self, alert):
        text = alert.text
        if 'Your wireless security mode is not compatible with' in text:
            alert.accept()
        elif 'WARNING: Your Wireless-N devices will only operate' in text:
            alert.accept()
        elif 'Your new setting will disable Wi-Fi Protected Setup.' in text:
            alert.accept()
        elif 'To use WEP security, WPS must be disabled. Proceed ?' in text:
             alert.accept()
        elif 'Warning ! Selecting None in Security Mode will make' in text:
             alert.accept()
        else:
           raise RuntimeError('Invalid handler')


    def get_number_of_pages(self):
        return 1


    def is_update_interval_supported(self):
        """
        Returns True if setting the PSK refresh interval is supported.

        @return True is supported; False otherwise
        """
        return False


    def is_visibility_supported(self):
        """
        Returns if AP supports setting the visibility (SSID broadcast).

        @return True if supported; False otherwise.

        """
        return False


    def get_supported_modes(self):
        return [{'band': ap_spec.BAND_2GHZ,
                 'modes': [ap_spec.MODE_B, ap_spec.MODE_G, ap_spec.MODE_N,
                           ap_spec.MODE_B | ap_spec.MODE_G,
                           ap_spec.MODE_G | ap_spec.MODE_N,
                           ap_spec.MODE_B | ap_spec.MODE_G | ap_spec.MODE_N]},
                {'band': ap_spec.BAND_5GHZ,
                 'modes': [ap_spec.MODE_A, ap_spec.MODE_N,
                           ap_spec.MODE_A | ap_spec.MODE_N]}]


    def get_supported_bands(self):
        return [{'band': ap_spec.BAND_2GHZ,
                 'channels': ['Auto', 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11]},
                {'band': ap_spec.BAND_5GHZ,
                 'channels': ['Auto', 36, 40, 44, 48, 149, 153, 157, 161, 165]}]


    def is_security_mode_supported(self, security_mode):
        """Check if the AP supports this mode of security.

        @param security_mode: Type of security.

        """
        return security_mode in (ap_spec.SECURITY_TYPE_DISABLED,
                                 ap_spec.SECURITY_TYPE_WPAPSK,
                                 ap_spec.SECURITY_TYPE_WPA2PSK,
                                 ap_spec.SECURITY_TYPE_WEP)


    def navigate_to_page(self, page_number):
        """Navigate to the required page.

        @param page_number: The page number to navigate to.

        """
        page_url = urlparse.urljoin(self.admin_interface_url, 'wlan.php')
        self.get_url(page_url, page_title='WESTERN DIGITAL')
        xpath_found = self.wait_for_objects_by_id(['loginusr', 'ssid'])
        if 'loginusr' in xpath_found:
            self._login_to_router()
        elif 'ssid' not in xpath_found:
            raise RuntimeError('The page %s did not load or Radio is switched'
                               'off' % page_url)
        self.set_radio(enabled=True)


    def _login_to_router(self):
        self.wait_for_object_by_id('loginusr')
        self.set_content_of_text_field_by_id('admin', 'loginusr',
                                             abort_check=True)
        self.wait_for_object_by_id('loginpwd')
        self.set_content_of_text_field_by_id('password', 'loginpwd',
                                             abort_check=True)
        self.click_button_by_xpath('//input[@value="Submit"]')
        self.wait_for_object_by_id('ssid')


    def save_page(self, page_number):
        """ Save page after applying settings.

        @param page_number: The page number to be saved.

        """
        self.wait_for_object_by_id('onsumit')
        self.click_button_by_id('onsumit', alert_handler=self._sec_alert)
        warning = '//h1[text()="Warning"]'
        settings_changed = True
        try:
            self.wait_for_object_by_xpath(warning)
            xpath = '//input[@id="onsumit"]'
            button = self.driver.find_elements_by_xpath(xpath)[1]
            button.click()
            self._handle_alert(xpath, self._sec_alert)
            self.wait_for_object_by_xpath('//input[@value="Ok"]', wait_time=5)
        except WebDriverException, e:
            logging.debug('There is a webdriver exception: "%s".', str(e))
            settings_changed = False
        if not settings_changed:
            try:
                # if settings are not changed, hit 'continue' button.
                self.driver.find_element_by_id('nochg')
                self.click_button_by_id('nochg')
            except WebDriverException, e:
                logging.debug('There is a webdriver exception: "%s".', str(e))


    def set_mode(self, mode, band=None):
        self.add_item_to_command_list(self._set_mode, (mode,), 1, 900)


    def _set_mode(self, mode, band=None):
        # This is a dummy wait to give enough time for the popup to
        # load all options.
        self._wait_for_page_reload()
        mode_mapping = {ap_spec.MODE_B | ap_spec.MODE_G:'Mixed 802.11 b+g',
                        ap_spec.MODE_G:'802.11g only',
                        ap_spec.MODE_B:'802.11b only',
                        ap_spec.MODE_N:'802.11n only',
                        ap_spec.MODE_A:'802.11a only',
                        ap_spec.MODE_G | ap_spec.MODE_N:'Mixed 802.11 g+n',
                        ap_spec.MODE_B | ap_spec.MODE_G | ap_spec.MODE_N:
                        'Mixed 802.11 b+g+n',
                        ap_spec.MODE_A | ap_spec.MODE_N: 'Mixed 802.11 a+n'}
        mode_id = 'wlan_mode'
        if self.current_band == ap_spec.BAND_5GHZ:
            mode_id = 'wlan_mode_Aband'
        mode_name = ''
        if mode in mode_mapping.keys():
            mode_name = mode_mapping[mode]
        else:
            raise RuntimeError('The mode selected \'%d\' is not supported by '
                               ' \'%s\'.', hex(mode), self.name)
        popup = self.wait_for_object_by_id(mode_id)
        while popup and not(self.object_by_id_exist(mode_id)):
            logging.debug('The object %s does not exist', mode_id)
        # Click is needed so that we can focus and don't get an empty popup.
        self.driver.find_element_by_id(mode_id).click()
        self.select_item_from_popup_by_id(mode_name, mode_id,
                                          alert_handler=self._sec_alert)


    def set_ssid(self, ssid):
        self.add_item_to_command_list(self._set_ssid, (ssid,), 1, 900)


    def _set_ssid(self, ssid):
        self._set_radio(True)
        ssid_id = 'ssid'
        if self.current_band == ap_spec.BAND_5GHZ:
            ssid_id = 'ssid_Aband'
        self.wait_for_object_by_id(ssid_id)
        self.set_content_of_text_field_by_id(ssid, ssid_id, abort_check=True)
        self._ssid = ssid


    def _wait_for_page_reload(self):
        """
        This router has a tendency to reload the webpage right after we load
        it. To avoid any exceptions because of this we wait for the page to
        reload by default.
        """
        elements = self.driver.find_elements_by_css_selector('span.checkbox')
        checkbox = elements[0]
        ssid_id = 'ssid'
        if self.current_band == ap_spec.BAND_5GHZ:
            checkbox = elements[3]
            ssid_id = 'ssid_Aband'
        for timer in range(5):   # Waiting for the page to reload
            try:
                if ('checkbox.png' in
                    checkbox.value_of_css_property('background-image')):
                    break
            except:
                pass
            time.sleep(1)
        try:
            if(self.wait_for_object_by_id(ssid_id).is_enabled()):
                logging.info('The page reload succeeded.')
        except:
            raise RuntimeError('The page reload after login failed.')


    def set_radio(self, enabled=True):
        self.add_item_to_command_list(self._set_radio, (enabled, ), 1, 1000)


    def _set_radio(self, enabled=True):
        self._wait_for_page_reload()
        elements = self.driver.find_elements_by_css_selector('span.checkbox')
        checkbox = elements[0]
        ssid = 'ssid'
        if self.current_band == ap_spec.BAND_5GHZ:
            checkbox = elements[3]
            ssid = 'ssid_Aband'
        image = 'checkbox_off.png'
        if enabled:
            image = 'checkbox.png'
            try:
                self.wait.until(ec.element_to_be_clickable((By.ID, ssid)))
            except TimeoutException, e:
                if not (image in
                        checkbox.value_of_css_property('background-image')):
                    checkbox.click()
                else:
                    message = 'Radio is not enabled. ' + str(e)
                    raise TimeoutException(message)
        elif not (image in checkbox.value_of_css_property('background-image')):
            checkbox.click()


    def set_channel(self, channel):
        self.add_item_to_command_list(self._set_channel, (channel,), 1, 900)


    def _set_channel(self, channel):
        self._wait_for_page_reload()
        position = self._get_channel_popup_position(channel)
        channel_id = 'channel'
        channel_choices = ['Auto', '2.412 GHz - CH 1', '2.417 GHz - CH 2',
                           '2.422 GHz - CH 3', '2.427 GHz - CH 4',
                           '2.432 GHz - CH 5', '2.437 GHz - CH 6',
                           '2.442 GHz - CH 7', '2.447 GHz - CH 8',
                           '2.452 GHz - CH 9', '2.457 GHz - CH 10',
                           '2.462 GHz - CH 11']
        if self.current_band == ap_spec.BAND_5GHZ:
            channel_id = 'channel_Aband'
            channel_choices = ['Auto', '5.180 GHz - CH 36', '5.200 GHz - CH 40',
                               '5.220 GHz - CH 44', '5.240 GHz - CH 48',
                               '5.745 GHz - CH 149', '5.765 GHz - CH 153',
                               '5.785 GHz - CH 157', '5.805 GHz - CH 161',
                               '5.825 GHz - CH 165']
        self.wait_for_object_by_id(channel_id)
        self.select_item_from_popup_by_id(channel_choices[position], channel_id)


    def set_channel_width(self, channel_wid):
        self.add_item_to_command_list(self._set_channel_width, (channel_wid,),
                                      1, 900)


    def _set_channel_width(self, channel_wid):
        channel_width_choice = ['20 MHz', '20/40 MHz(Auto)']
        width_id = 'bw'
        if self.current_band == ap_spec.BAND_5GHZ:
            width_id = 'bw_Aband'
        self.select_item_from_popup_by_id(channel_width_choice[channel_wid],
                                          width_id)


    def set_band(self, band):
        if band == ap_spec.BAND_5GHZ:
            self.current_band = ap_spec.BAND_5GHZ
        elif band == ap_spec.BAND_2GHZ:
            self.current_band = ap_spec.BAND_2GHZ
        else:
            raise RuntimeError('Invalid band sent %s' % band)
        self.set_radio(True)


    def _set_security(self, security_type, wait_path=None):
        self._wait_for_page_reload()
        sec_id = 'security_type'
        if self.current_band == ap_spec.BAND_5GHZ:
            sec_id = 'security_type_Aband'
            text = '//input[@name="wpapsk_Aband" and @type="text"]'
        self.wait_for_object_by_id(sec_id, wait_time=5)
        if self.item_in_popup_by_id_exist(security_type, sec_id):
            self.select_item_from_popup_by_id(security_type, sec_id,
                                              wait_for_xpath=wait_path,
                                              alert_handler=self._sec_alert)
        elif security_type == 'WEP':
            raise RuntimeError('Could not find WEP security_type in dropdown. '
                               'Please check the network mode. '
                               'Some of the modes do not support WEP.')
        else:
            raise RuntimeError('The dropdown %s does not have item %s' %
                               (sec_id, security_type))


    def set_security_disabled(self):
        self.add_item_to_command_list(self._set_security_disabled, (), 1, 1000)


    def _set_security_disabled(self):
        self._set_security('None')


    def set_security_wep(self, key_value, authentication):
        self.add_item_to_command_list(self._set_security_wep,
                                      (key_value, authentication), 1, 1000)


    def _set_security_wep(self, key_value, authentication):
        # WEP is not supported for Wireless-N only and Mixed (g+n, b+g+n) mode.
        # WEP does not show up in the list, no alert is thrown.
        text = '//input[@name="wepkey_64"]'
        if self.current_band == ap_spec.BAND_5GHZ:
            text = '//input[@name="wepkey_64_Aband"]'
        self._set_security('WEP', text)
        self.set_content_of_text_field_by_xpath(key_value, text,
                                                abort_check=True)


    def set_security_wpapsk(self, security, shared_key, update_interval=None):
        # WEP and WPA-Personal are not supported for Wireless-N only mode,
        self.add_item_to_command_list(self._set_security_wpapsk,
                                      (security, shared_key,), 1, 1000)


    def _set_security_wpapsk(self, security, shared_key):
        text = 'wpapsk'
        if self.current_band == ap_spec.BAND_5GHZ:
            text = 'wpapsk_Aband'
        if security == ap_spec.SECURITY_TYPE_WPAPSK:
            self._set_security('WPA - Personal', '//input[@id="%s"]' % text)
        else:
            self._set_security('WPA2 - Personal', '//input[@id="%s"]' % text)
        self.set_content_of_text_field_by_id(shared_key, text,
                                             abort_check=False)


    def set_visibility(self, visible=True):
        # The SSID Broadcast can't be reliable set on this AP beacuse the CSS
        # property for backgroung-image always returns OFF.
        return None
