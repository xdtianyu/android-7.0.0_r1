# Copyright (c) 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import urlparse

import ap_spec
import linksyse_dual_band_configurator


class LinksysWRT600APConfigurator(linksyse_dual_band_configurator.
                                  LinksyseDualBandAPConfigurator):
    """Derived class to control Linksys wrt600 router."""


    def _sec_alert(self, alert):
        text = alert.text
        if 'Your wireless security mode is not compatible with' in text:
            alert.accept()
        elif 'WARNING: Your Wireless-N devices will only operate' in text:
            alert.accept()
        else:
            self._alert_handler(alert)


    def get_number_of_pages(self):
        return 2


    def get_supported_modes(self):
        return [{'band': ap_spec.BAND_2GHZ,
                 'modes': [ap_spec.MODE_B, ap_spec.MODE_N,
                           ap_spec.MODE_G, ap_spec.MODE_M]},
                {'band': ap_spec.BAND_5GHZ,
                 'modes': [ap_spec.MODE_A, ap_spec.MODE_N, ap_spec.MODE_M]}]


    def is_security_mode_supported(self, security_mode):
        """Returns if the passed in security mode is supported.

        @param security_mode: one of the supported security methods defined
                              in APSpec.

        @returns True is suppported; False otherwise.
        """
        return security_mode in (ap_spec.SECURITY_TYPE_DISABLED,
                                 ap_spec.SECURITY_TYPE_WPAPSK,
                                 ap_spec.SECURITY_TYPE_WPA2PSK,
                                 ap_spec.SECURITY_TYPE_WEP)


    def navigate_to_page(self, page_number):
        """Navigate to the passed in page.

        @param page_number: the page number as an integer
        """
        if page_number == 1:
            page_url = urlparse.urljoin(self.admin_interface_url,
                                        'BasicWirelessSettings.htm')
            self.get_url(page_url, page_title='Settings')
        elif page_number == 2:
            page_url = urlparse.urljoin(self.admin_interface_url,
                                        'WirelessSecurity.htm')
            self.get_url(page_url, page_title='Security')
        else:
            raise RuntimeError('Invalid page number passed. Number of pages '
                               '%d, page value sent was %d' %
                               (self.get_number_of_pages(), page_number))


    def save_page(self, page_number):
        self.driver.switch_to_default_content()
        submit_btn = '//a[@href="javascript:ValidateForm(document.forms[0]);"]'
        if page_number == 1:
            submit_btn='//a[@href="javascript:Try_Submit(document.forms[0]);"]'
        self.click_button_by_xpath(submit_btn,
                                   alert_handler=self._alert_handler)
        self.wait_for_object_by_xpath(submit_btn, wait_time=20)


    def _set_mode(self, mode, band=None):
        mode_mapping = {ap_spec.MODE_N: ' Wireless-N Only',
                        ap_spec.MODE_M: ' Mixed',
                        'Disabled': ' Disabled'}
        xpath = '//select[@name="wl_mode"]'
        if self.current_band == ap_spec.BAND_2GHZ:
            mode_mapping[ap_spec.MODE_G] = ' Wireless-G Only'
            mode_mapping[ap_spec.MODE_B] = ' Wireless-B Only'
        elif self.current_band == ap_spec.BAND_5GHZ:
            xpath = '//select[@name="wl_mode_1"]'
            mode_mapping[ap_spec.MODE_A] = 'Wireless-A Only'
            mode_mapping[ap_spec.MODE_N] = 'Wireless-N Only'
        mode_name = ''
        if mode in mode_mapping.keys():
            mode_name = mode_mapping[mode]
        else:
            raise RuntimeError('The mode selected %d is not supported by router'
                               ' %s.', hex(mode), self.name)
        self.select_item_from_popup_by_xpath(mode_name, xpath)


    def _set_ssid(self, ssid):
        xpath = '//input[@name="wl_ssid"]'
        if self.current_band == ap_spec.BAND_5GHZ:
            xpath = '//input[@name="wl_ssid_1"]'
        self.set_content_of_text_field_by_xpath(ssid, xpath)
        self._ssid = ssid


    def _set_channel(self, channel):
        position = self._get_channel_popup_position(channel)
        channel_choices = ['Auto',
                           '1 - 2.412GHz', '2 - 2.417GHz', '3 - 2.422GHz',
                           '4 - 2.427GHz', '5 - 2.432GHz', '6 - 2.437GHz',
                           '7 - 2.442GHz', '8 - 2.447GHz', '9 - 2.452GHz',
                           '10 - 2.457GHz', '11 - 2.462GHz']
        xpath = '//select[@name="wl_channel"]'
        if self.current_band == ap_spec.BAND_5GHZ:
            xpath = '//select[@name="wl_channel_1"]'
            channel_choices = ['Auto (DFS)',
                               '36 - 5.180GHz', '40 - 5.200GHz',
                               '44 - 5.220GHz', '48 - 5.240GHz',
                               '149 - 5.745GHz', '153 - 5.765GHz',
                               '157 - 5.785GHz', '161 - 5.805GHz']
        self.select_item_from_popup_by_xpath(channel_choices[position],
                                             xpath)


    def _set_security(self, sec_type, look_for=None):
        xpath = ('//select[@name="wl_security" and \
                  @onchange="WirelessSecurityType ( this.value )"]')
        self.driver.switch_to_default_content()
        self.driver.switch_to_frame('SSIDAuthMode')
        if self.current_band == ap_spec.BAND_5GHZ:
            xpath = ('//select[@name="wl_security" and \
                      @onchange="WirelessSecurityType1 ( this.value )"]')
            self.driver.switch_to_default_content()
            self.driver.switch_to_frame('SSIDAuthMode1')
        self.wait_for_object_by_xpath(xpath)
        self.select_item_from_popup_by_xpath(sec_type, xpath,
                                             wait_for_xpath=look_for,
                                             alert_handler=self._sec_alert)


    def set_security_disabled(self):
        self.add_item_to_command_list(self._set_security_disabled, (), 2, 900)


    def _set_security_disabled(self):
        self._set_security(' Disabled')


    def set_security_wep(self, key_value, authentication):
        self.add_item_to_command_list(self._set_security_wep,
                                      (key_value, authentication), 2, 900)


    def _set_security_wep(self, key_value, authentication):
        text_field = '//input[@name="passphrase_key"]'
        xpath = '//input[@name="GenerateBtn"]'
        self._set_security(' WEP', look_for=text_field)
        self.set_content_of_text_field_by_xpath(key_value, text_field,
                                                abort_check=True)
        self.click_button_by_xpath(xpath, alert_handler=self._sec_alert)
        self.wait_for_object_by_xpath('//input[@name="wl_key4"]')


    def set_security_wpapsk(self, security, shared_key, update_interval=None):
        self.add_item_to_command_list(self._set_security_wpapsk,
                                      (security, shared_key, update_interval),
                                       2, 900)


    def _set_security_wpapsk(self, security, shared_key, update_interval=None):
        if security == ap_spec.SECURITY_TYPE_WPAPSK:
            wpa_item = ' WPA-Personal'
        else:
            wpa_item = ' WPA2-Personal'
        key_field = '//input[@name="wl_wpa_psk"]'
        self._set_security(wpa_item, look_for=key_field)
        self.set_content_of_text_field_by_xpath(shared_key, key_field,
                                                abort_check=True)
        if update_interval:
            self.set_content_of_text_field_by_xpath(
                                    '//input[@name="key_renewal"]', key_field,
                                    abort_check=True)


    def is_update_interval_supported(self):
        """
        Returns True if setting the PSK refresh interval is supported.

        @return True is supported; False otherwise
        """
        return True


    def _set_visibility(self, visible=True):
        int_value = 0 if visible else 1
        xpath = ('//input[@value="%d" and @name="wl_hide_ssid"]' % int_value)
        self.click_button_by_xpath(xpath)
