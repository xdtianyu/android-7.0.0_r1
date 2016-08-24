# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import time
import urlparse

import dynamic_ap_configurator
import ap_spec
from selenium.common.exceptions import TimeoutException as \
SeleniumTimeoutException


class TrendnetAPConfigurator(
        dynamic_ap_configurator.DynamicAPConfigurator):
    """Derived class to control the Trendnet TEW-639GR."""


    def _alert_handler(self, alert):
        """Checks for any modal dialogs which popup to alert the user and
        either raises a RuntimeError or ignores the alert.

        Args:
          alert: The modal dialog's contents.
        """
        text = alert.text
        if 'Please input 10 or 26 characters of WEP key1' in text:
            alert.accept()
            raise RuntimeError(text)


    def get_number_of_pages(self):
        return 2


    def get_supported_bands(self):
        return [{'band': ap_spec.BAND_2GHZ, 'channels': range(1, 12)}]


    def get_supported_modes(self):
        return [{'band': ap_spec.BAND_2GHZ,
                 'modes': [ap_spec.MODE_N,
                           ap_spec.MODE_B | ap_spec.MODE_G,
                           ap_spec.MODE_B | ap_spec.MODE_G | ap_spec.MODE_N]}]


    def is_security_mode_supported(self, security_mode):
        return security_mode in (ap_spec.SECURITY_TYPE_DISABLED,
                                 ap_spec.SECURITY_TYPE_WEP,
                                 ap_spec.SECURITY_TYPE_WPAPSK,
                                 ap_spec.SECURITY_TYPE_WPA2PSK)


    def navigate_to_page(self, page_number):
        # All settings are on the same page, so we always open the config page
        if page_number == 1:
            page_url = urlparse.urljoin(self.admin_interface_url,
                                        'wireless/basic.asp')
            self.get_url(page_url, page_title='TEW')
            ids_list = ['display_SSID1', 'sz11gChannel']
        elif page_number == 2:
            page_url = urlparse.urljoin(self.admin_interface_url,
                                        'wireless/security.asp')
            self.get_url(page_url, page_title='TEW')
            ids_list = ['ssidIndex', 'security_mode']
        else:
            raise RuntimeError('Invalid page number passed.  Number of pages '
                               '%d, page value sent was %d' %
                               (self.get_number_of_pages(), page_number))
        try:
            self.wait_for_objects_by_id(ids_list, wait_time=30)
        except SeleniumTimeoutException, e:
            logging.info('Page did not load completely. Refresh driver')
            self.driver.refresh()
            self.wait_for_objects_by_id(ids_list, wait_time=30)


    def wait_for_progress_bar(self):
        """Watch the progress bar and wait for up to two minutes."""
        for i in xrange(240):
            if not self.object_by_id_exist('progressValue'):
                return
            try:
                progress_value = self.wait_for_object_by_id('progressValue')
                html = self.driver.execute_script(
                        'return arguments[0].innerHTML', progress_value)
            except Exception as e:
                # The progress bar is gone and the page reloaded.  This can
                # happen if we fly from < 95% to done.
                return
            percentage = html.rstrip('%')
            if int(percentage) < 95:
                time.sleep(0.5)
            else:
                break


    def save_page(self, page_number):
        if page_number == 1:
            xpath = ('//input[@type="submit" and @value="Apply"]')
        elif page_number == 2:
            xpath = ('//input[@class="button_submit" and @value="Apply"]')
        self.click_button_by_xpath(xpath, alert_handler=self._alert_handler)
        # Wait for the settings progress bar to save the setting.
        self.wait_for_progress_bar()


    def set_mode(self, mode, band=None):
        self.add_item_to_command_list(self._set_mode, (mode,), 1, 100)


    def _set_mode(self, mode, band=None):
        # Different bands are not supported so we ignore.
        # Create the mode to popup item mapping
        mode_mapping = {ap_spec.MODE_B | ap_spec.MODE_G | ap_spec.MODE_N:
                        '2.4GHz 802.11 b/g/n mixed mode',
                        ap_spec.MODE_N: '2.4GHz 802.11 n only',
                        ap_spec.MODE_B | ap_spec.MODE_G:
                        '2.4GHz 802.11 b/g mixed mode'}
        mode_name = ''
        if mode in mode_mapping.keys():
            mode_name = mode_mapping[mode]
        else:
            raise RuntimeError('The mode selected %d is not supported by router'
                               ' %s.', hex(mode), self.name)
        self.select_item_from_popup_by_id(mode_name, 'wirelessmode',
                                          wait_for_xpath='id("wds_mode")')


    def set_radio(self, enabled=True):
        logging.debug('Enabling/Disabling the radio is not supported on this '
                      'router (%s).', self.name)
        return None


    def set_ssid(self, ssid):
        self.add_item_to_command_list(self._set_ssid, (ssid,), 1, 100)


    def _set_ssid(self, ssid):
        self.set_content_of_text_field_by_id(ssid, 'display_SSID1')
        self._ssid = ssid


    def set_channel(self, channel):
        self.add_item_to_command_list(self._set_channel, (channel,), 1, 100)


    def _set_channel(self, channel):
        position = self._get_channel_popup_position(channel)
        channel_choices = ['2412MHz (Channel 1)', '2417MHz (Channel 2)',
                           '2422MHz (Channel 3)', '2427MHz (Channel 4)',
                           '2432MHz (Channel 5)', '2437MHz (Channel 6)',
                           '2442MHz (Channel 7)', '2447MHz (Channel 8)',
                           '2452MHz (Channel 9)', '2457MHz (Channel 10)',
                           '2462MHz (Channel 11)']
        self.select_item_from_popup_by_id(channel_choices[position],
                                          'sz11gChannel')


    def set_band(self, band):
        return None


    def set_security_disabled(self):
        self.add_item_to_command_list(self._set_security_disabled, (), 2, 1000)


    def _set_security_disabled(self):
        self.wait_for_object_by_id('security_mode')
        self.select_item_from_popup_by_id('Disable', 'security_mode')


    def set_security_wep(self, key_value, authentication):
        self.add_item_to_command_list(self._set_security_wep,
                                      (key_value, authentication), 2, 900)


    def _set_security_wep(self, key_value, authentication):
        text_field = 'id("WEP1")'
        popup_id = 'security_mode'
        self.wait_for_object_by_id(popup_id)
        self.select_item_from_popup_by_id('WEP-OPEN',
                                          popup_id, wait_for_xpath=text_field)
        self.set_content_of_text_field_by_xpath(key_value, text_field,
                                                abort_check=True)


    def set_security_wpapsk(self, security, shared_key, update_interval=1800):
        self.add_item_to_command_list(self._set_security_wpapsk,
                                      (security, shared_key, update_interval),
                                       2, 900)


    def _set_security_wpapsk(self, security, shared_key, update_interval=1800):
        self.wait_for_object_by_id('security_mode')
        if security == ap_spec.SECURITY_TYPE_WPAPSK:
            wpa_item = 'WPA-PSK'
        else:
            wpa_item = 'WPA2-PSK'
        self.select_item_from_popup_by_id(wpa_item, 'security_mode',
                                          wait_for_xpath='id("passphrase")')
        self.set_content_of_text_field_by_id(shared_key, 'passphrase')
        self.set_content_of_text_field_by_id(update_interval,
                                             'keyRenewalInterval')


    def set_visibility(self, visible=True):
        self.add_item_to_command_list(self._set_visibility, (visible,), 1, 100)


    def _set_visibility(self, visible=True):
        # value=1 is visible; value=0 is invisible
        int_value = int(visible)
        xpath = ('//input[@value="%d" and @name="broadcastssid"]' % int_value)
        self.click_button_by_xpath(xpath)
