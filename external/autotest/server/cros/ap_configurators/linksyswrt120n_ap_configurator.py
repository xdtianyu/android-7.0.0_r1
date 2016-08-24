# Copyright (c) 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import urlparse
import ap_spec
import linksyse_single_band_configurator


class LinksysWRT120NAPConfigurator(linksyse_single_band_configurator.
                                   LinksyseSingleBandAPConfigurator):
    """Derived class to control Linksys WRT 120N router."""


    def navigate_to_page(self, page_number):
        if page_number == 1:
            page_url = urlparse.urljoin(self.admin_interface_url,
                    'index.stm?title=Wireless-Basic%20Wireless%20Settings')
            self.get_url(page_url, page_title='Settings')
        elif page_number == 2:
            page_url = urlparse.urljoin(self.admin_interface_url,
                    'index.stm?title=Wireless-Wireless%20Security')
            self.get_url(page_url, page_title='Security')
        else:
            raise RuntimeError('Invalid page number passed. Number of pages '
                               '%d, page value sent was %d' %
                               (self.get_number_of_pages(), page_number))


    def save_page(self, page_number):
        save_button = '//input[@name="saveButton"]'
        self.click_button_by_xpath(save_button, alert_handler=self._sec_alert)


    def _set_mode(self, mode, band=None):
        mode_mapping = {ap_spec.MODE_M:'Mixed',
                        ap_spec.MODE_B | ap_spec.MODE_G:'BG-Mixed',
                        ap_spec.MODE_G:'Wireless-G Only',
                        ap_spec.MODE_B:'Wireless-B Only',
                        ap_spec.MODE_N:'Wireless-N Only',
                        'Disabled':' Disabled'}
        mode_name = mode_mapping.get(mode)
        if not mode_name:
            raise RuntimeError('The mode %d not supported by router %s. ',
                               hex(mode), self.name)
        xpath = '//select[@name="op_mode"]'
        self.select_item_from_popup_by_xpath(mode_name, xpath,
                                             alert_handler=self._sec_alert)


    def _set_ssid(self, ssid):
        xpath = '//input[@name="wl_ssid"]'
        self.set_content_of_text_field_by_xpath(ssid, xpath, abort_check=False)
        self._ssid = ssid


    def _set_channel(self, channel):
        position = self._get_channel_popup_position(channel)
        xpath = '//select[@name="channel"]'
        channels = ['Auto',
                    '1 - 2.412GHz', '2 - 2.417GHz', '3 - 2.422GHz',
                    '4 - 2.427GHz', '5 - 2.432GHz', '6 - 2.437GHz',
                    '7 - 2.442GHz', '8 - 2.447GHz', '9 - 2.452GHz',
                    '10 - 2.457GHz', '11 - 2.462GHz']
        self.select_item_from_popup_by_xpath(channels[position], xpath)


    def _set_security_disabled(self):
        xpath = '//select[@name="sec_mode"]'
        self.select_item_from_popup_by_xpath('Disabled', xpath)


    def _set_security_wep(self, key_value, authentication):
        popup = '//select[@name="sec_mode"]'
        self.select_item_from_popup_by_xpath(' WEP ', popup,
                                             alert_handler=self._sec_alert)
        text = '//input[@name="passPhrase"]'
        self.set_content_of_text_field_by_xpath(key_value, text,
                                                abort_check=True)
        xpath = '//input[@name="generate_key"]'
        self.click_button_by_xpath(xpath, alert_handler=self._sec_alert)


    def _set_security_wpapsk(self, security, shared_key, update_interval=None):
        popup = '//select[@name="sec_mode"]'
        self.wait_for_object_by_xpath(popup)
        if security == ap_spec.SECURITY_TYPE_WPAPSK:
            wpa_item = 'WPA Personal'
        else:
            wpa_item = 'WPA2 Personal'
        self.select_item_from_popup_by_xpath(wpa_item, popup,
                                             alert_handler=self._sec_alert)
        text = '//input[@name="sharedkey"]'
        self.set_content_of_text_field_by_xpath(shared_key, text,
                                                abort_check=True)
        key = '//input[@name="group_key_second"]'
        if update_interval:
            self.set_content_of_text_field_by_xpath(shared_key, key,
                                                    abort_check=True)


    def is_update_interval_supported(self):
        """
        Returns True if setting the PSK refresh interval is supported.

        @return True is supported; False otherwise
        """
        return True


    def _set_visibility(self, visible=True):
        int_value = 1 if visible else 0
        xpath = ('//input[@value="%d" and @name="wlan_broadcast"]' % int_value)
        self.click_button_by_xpath(xpath, alert_handler=self._sec_alert)
