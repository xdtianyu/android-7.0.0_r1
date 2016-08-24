# Copyright (c) 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import dynamic_ap_configurator
import ap_spec


class Keeboxw150nrAPConfigurator(
        dynamic_ap_configurator.DynamicAPConfigurator):
    """Derived class to control the Keeboxw150nr AP."""


    def navigate_to_page(self, page_number):
        """
        Navigate to the required page.
        """
        if page_number == 1:
            page_url = os.path.join(self.admin_interface_url ,'wlan2_basic.php')
        elif page_number == 2:
            page_url = os.path.join(self.admin_interface_url ,
                                    'wlan2_security.php')
        else:
            raise RuntimeError('Invalid page number passed.  Number of pages is'
                               '%d, page value sent was %d' %
                               (self.get_number_of_pages(), page_number))
        self.get_url(page_url, page_title='HOME')
        # Check if we have to login
        if self.admin_login_needed(page_url):
            self.ap_login()


    def admin_login_needed(self, page_url):
        """
        Check if we are on the admin login page.

        @param page_url: string, the page to open.

        @return True if login needed False otherwise.
        """
        login_element = '//input[@value="Login"]'
        apply_element = '//input[@value="Apply"]'
        login_displayed = self.wait_for_objects_by_xpath([login_element,
                                                          apply_element])
        if login_displayed == login_element:
            return True
        elif login_displayed == apply_element:
            return False
        else:
            raise Exception('The page %s did not load' % page_url)


    def ap_login(self):
        """
        Login as admin before configuring settings.
        """
        self.set_content_of_text_field_by_id('admin', 'loginusr',
                                             abort_check=True)
        self.set_content_of_text_field_by_id('password', 'loginpwd',
                                             abort_check=True)
        self.click_button_by_xpath('//input[@value="Login"]')
        self.wait_for_object_by_id('content')


    def save_page(self,page_number):
        xpath_apply = ('//input[@type="button" and @value="Apply"]')
        xpath_continue = ('//input[@type="button" and @value="Continue"]')
        self.click_button_by_xpath(xpath_apply,
                                   alert_handler=self._alert_handler)
        try:
            element = self.wait_for_object_by_xpath(xpath_continue)
            self.click_button_by_xpath(xpath_continue,
                                       alert_handler=self._alert_handler)
        except:
            self._handle_alert(xpath_apply, self._alert_handler)
        self.wait_for_object_by_xpath(xpath_apply)


    def get_number_of_pages(self):
        return 2


    def set_ssid(self, ssid):
        self.add_item_to_command_list(self._set_ssid, (ssid,), 1, 1000)


    def _set_ssid(self, ssid):
        self.set_content_of_text_field_by_id(ssid, 'ssid')
        self._ssid = ssid


    def set_radio(self, enabled=True):
        self.add_item_to_command_list(self._set_radio, (enabled, ), 1, 1000)


    def _set_radio(self, enabled=True):
        # value=1 is ON; value=0 is OFF
        int_value = int(enabled)
        xpath = ('//input[@value="%d" and @name="en_wifi"]' %
                 int_value)
        self.click_button_by_xpath(xpath, alert_handler=self._alert_handler)


    def is_update_interval_supported(self):
        """
        Returns True if setting the PSK refresh interval is supported.

        @return True is supported; False otherwise
        """
        return False


    def set_band(self, band):
        return None


    def is_security_mode_supported(self, security_mode):
        return security_mode in (ap_spec.SECURITY_TYPE_DISABLED,
                                 ap_spec.SECURITY_TYPE_WEP,
                                 ap_spec.SECURITY_TYPE_WPAPSK,
                                 ap_spec.SECURITY_TYPE_WPA2PSK)


    def get_supported_bands(self):
        return [{'band': ap_spec.BAND_2GHZ, 'channels': range(1, 12)}]


    def set_security_disabled(self):
        self.add_item_to_command_list(self._set_security_disabled, (), 2, 1000)


    def _set_security_disabled(self):
        self.wait_for_object_by_id('security_type')
        self.select_item_from_popup_by_id('Disable', 'security_type')

    def get_supported_modes(self):
        return [{'band': ap_spec.BAND_2GHZ,
                 'modes': [ap_spec.MODE_B,
                           ap_spec.MODE_G,
                           ap_spec.MODE_N,
                           ap_spec.MODE_B | ap_spec.MODE_G,
                           ap_spec.MODE_B | ap_spec.MODE_G | ap_spec.MODE_N]}]


    def set_mode(self, mode, band=None):
        self.add_item_to_command_list(self._set_mode, (mode,), 1, 1000)


    def _set_mode(self, mode, band=None):
        # Different bands are not supported so we ignore.
        # Create the mode to popup item mapping
        mode_mapping = {ap_spec.MODE_B | ap_spec.MODE_G | ap_spec.MODE_N:
                        '2.4 GHz (802.11b/g/n)',
                        ap_spec.MODE_N: '2.4 GHz (802.11n)',
                        ap_spec.MODE_B: '2.4 GHz (802.11b)',
                        ap_spec.MODE_G: '2.4 GHz (802.11g)',
                        ap_spec.MODE_B | ap_spec.MODE_G:
                        '2.4 GHz (802.11b/g)'}
        mode_name = ''
        if mode in mode_mapping.keys():
            mode_name = mode_mapping[mode]
        else:
            raise RuntimeError('The mode selected %d is not supported by router'
                               ' %s.', hex(mode), self.name)
        self.select_item_from_popup_by_id(mode_name, 'wlan_mode')


    def get_supported_bands(self):
        return [{'band': ap_spec.BAND_2GHZ, 'channels': range(1, 11)}]


    def set_channel(self, channel):
        self.add_item_to_command_list(self._set_channel, (channel,), 1, 900)


    def _set_channel(self, channel):
        position = self._get_channel_popup_position(channel)
        channel_choices = ['1', '2', '3', '4', '5',
                           '6', '7', '8', '9', '10', '11']
        self.select_item_from_popup_by_id(channel_choices[position],
                                          'channel')


    def set_visibility(self, visible=True):
        self.add_item_to_command_list(self._set_visibility, (visible,), 2, 1000)


    def _set_visibility(self, visible=True):
        # value=1 is visible; value=0 is invisible
        if visible == True:
           self.select_item_from_popup_by_id('Enable', 'suppress')
        else:
           self.select_item_from_popup_by_id('Disable', 'suppress')


    def set_security_wep(self, key_value, authentication):
        self.add_item_to_command_list(self._set_security_wep,
                                      (key_value, authentication), 2, 900)


    def _set_security_wep(self, key_value, authentication):
        # This AP does not allow selecting Shared Key. Open System is default.
        self.wait_for_object_by_id('security_type')
        self.select_item_from_popup_by_id('WEP', 'security_type')
        self.select_item_from_popup_by_id('64-bit', 'wep_key_len')
        self.select_item_from_popup_by_id('ASCII (5 characters)',
                                          'key_str_type')
        self.set_content_of_text_field_by_id(key_value, 'wep_key_1')


    def set_security_wpapsk(self, security, shared_key, update_interval=None):
        self.add_item_to_command_list(self._set_security_wpapsk,
                                      (security,shared_key, update_interval),
                                       2, 900)


    def _set_security_wpapsk(self, security, shared_key, update_interval=None):
        self.wait_for_object_by_id('security_type')
        if security == ap_spec.SECURITY_TYPE_WPAPSK:
            self.select_item_from_popup_by_id('WPA Only', 'security_type')
        else:
            self.select_item_from_popup_by_id('WPA2 Only', 'security_type')
        self.select_item_from_popup_by_id('Passphrase', 'id_shared_keytype')
        self.set_content_of_text_field_by_id(shared_key, 'wpapsk')


    def _alert_handler(self, alert):
        """Checks for any modal dialogs which popup to alert the user and
        either raises a RuntimeError or ignores the alert.

        Args:
          alert: The modal dialog's contents.
        """
        text = alert.text
        if 'Your wireless settings have changed' in text:
            alert.accept()
        elif 'Settings have not changed' in text:
            alert.accept()
        else:
            raise RuntimeError('We have an unhandled alert: %s' % text)

