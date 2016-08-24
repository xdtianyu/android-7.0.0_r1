# Copyright (c) 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Class to control the Dlink_DIR505LAP router."""

import urlparse
import logging
import time

import dlink_ap_configurator
import ap_spec

class DLinkDIR505lAPConfigurator(
        dlink_ap_configurator.DLinkAPConfigurator):
    """Derived class to control the Dlink_DIR505lAP router."""


    def _alert_handler(self, alert):
        """Checks for any modal dialogs which popup to alert the user and
        either raises a RuntimeError or ignores the alert.

        Args:
          alert: The modal dialog's contents.
        """
        text = alert.text
        if 'Nothing has changed, save anyway?' in text:
            alert.accept()
        elif 'To hidden SSID will be disabled the WPS, Are you sure ?' in text:
            alert.accept()
        elif 'Open mode configuration is not secure.' in text:
            alert.accept()
        else:
            raise RuntimeError('We have an unhandled alert: %s' % text)


    def get_number_of_pages(self):
        return 1


    def get_supported_bands(self):
        return [{'band': ap_spec.BAND_2GHZ,
                 'channels': [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11]}]


    def get_supported_modes(self):
        return [{'band': ap_spec.BAND_2GHZ,
                 'modes': [ap_spec.MODE_N, ap_spec.MODE_G,
                           ap_spec.MODE_N | ap_spec.MODE_G,
                           ap_spec.MODE_N | ap_spec.MODE_G | ap_spec.MODE_B]}]


    def is_security_mode_supported(self, security_mode):
        """
        Returns if a given security_type is supported.

        @param security_mode: one security modes defined in the APSpec

        @return True if the security mode is supported; False otherwise.

        """
        return security_mode in (ap_spec.SECURITY_TYPE_DISABLED,
                                 ap_spec.SECURITY_TYPE_WPAPSK,
                                 ap_spec.SECURITY_TYPE_WPA2PSK)


    def navigate_to_page(self, page_number):
        """
        Navigates to the page corresponding to the given page number.

        This method performs the translation between a page number and a url to
        load. This is used internally by apply_settings.

        @param page_number: page number of the page to load

        """
        page_url = urlparse.urljoin(self.admin_interface_url, 'Wireless.htm')
        self.get_url(page_url, page_title='D-LINK SYSTEMS')
        if not self.object_by_xpath_exist('//input[@id="user_pwd"]'):
            # We are at the config page, done.
            return
        self.set_content_of_text_field_by_id('password', 'user_pwd')
        self.click_button_by_id('login')


    def save_page(self, page_number):
        """
        Saves the given page.

        @param page_number: Page number of the page to save.

        """
        # All settings are on the same page, we can ignore page_number
        self.click_button_by_id('apply_btn', alert_handler=self._alert_handler)
        # Second alert may pop-up, so we must send it to our alert handler
        try:
            alert = self.driver.switch_to_alert()
            self._handler(self._alert_handler)
        except:
            logging.debug("No alert present")
        timer = self.wait_for_object_by_id('show_second', wait_time=20)
        while (self.object_by_id_exist('show_second') and
               int(timer.text) > 5):
            time.sleep(1)
        self.click_button_by_id('back_btn', alert_handler=self._alert_handler)


    def set_mode(self, mode, band=None):
        # Mode overrides the band.  So if a band change is made after a mode
        # change it may make an incompatible pairing.
        self.add_item_to_command_list(self._set_mode, (mode, band), 1, 800)


    def _set_mode(self, mode, band=None):
        # Create the mode to popup item mapping
        mode_mapping = {ap_spec.MODE_N: '802.11n only',
            ap_spec.MODE_G: 'Mixed 802.11n, 802.11g',
            ap_spec.MODE_N | ap_spec.MODE_G: 'Mixed 802.11n, 802.11g',
            ap_spec.MODE_N | ap_spec.MODE_G | ap_spec.MODE_B:
            'Mixed 802.11n, 802.11g and 802.11b'}
        if mode in mode_mapping.keys():
            popup_value = mode_mapping[mode]
        else:
            raise RuntimeError('The mode selected %s is not '
                               'supported by router %s.' %
                               (hex(mode), self.name))
        self.select_item_from_popup_by_id(popup_value, 'dot11_mode')


    def set_radio(self, enabled=True):
        # If we are enabling we are activating all other UI components, do
        # it first. Otherwise we are turning everything off so do it last.
        logging.debug('This router (%s) does not support enabling/disabling '
                      'wireless', self.name)
        return None


    def set_ssid(self, ssid):
        # Can be done as long as it is enabled
        self.add_item_to_command_list(self._set_ssid, (ssid,), 1, 900)


    def _set_ssid(self, ssid):
        self.set_content_of_text_field_by_id(ssid, 'ssid')
        self._ssid = ssid


    def set_channel(self, channel):
        self.add_item_to_command_list(self._set_channel, (channel,), 1, 900)


    def _set_channel(self, channel):
        position = self._get_channel_popup_position(channel)
        channel_choices = ['2.412 GHz - CH 1', '2.417 GHz - CH 2',
                           '2.422 GHz - CH 3', '2.427 GHz - CH 4',
                           '2.432 GHz - CH 5', '2.437 GHz - CH 6',
                           '2.442 GHz - CH 7', '2.447 GHz - CH 8',
                           '2.452 GHz - CH 9', '2.457 GHz - CH 10',
                           '2.462 GHz - CH 11']
        channel_popup = self.driver.find_element_by_id('channel')
        if channel_popup.get_attribute('disabled') == 'true':
            self.set_check_box_selected_by_id('autochann', selected=False)
        self.select_item_from_popup_by_id(channel_choices[position], 'channel')


    def set_band(self, band):
        logging.debug('This router (%s) does not support multiple bands.',
                       self.name)
        return None


    def set_security_disabled(self):
        self.add_item_to_command_list(self._set_security_disabled, (), 1, 900)


    def _set_security_disabled(self):
        self.select_item_from_popup_by_id('None', 'security_mode')


    def set_security_wep(self, key_value, authentication):
        logging.debug('This router (%s) does not support WEP', self.name)
        return None


    def set_security_wpapsk(self, security, shared_key, update_interval=1800):
        self.add_item_to_command_list(self._set_security_wpapsk,
                                     (security, shared_key, update_interval),
                                      1, 900)


    def _set_security_wpapsk(self, security, shared_key, update_interval=1800):
        self.select_item_from_popup_by_id('WPA-Personal', 'security_mode')
        if security == ap_spec.SECURITY_TYPE_WPAPSK:
            wpa_item = 'WPA Only'
        else:
            wpa_item = 'WPA2 Only'
        self.select_item_from_popup_by_id(wpa_item, 'wpa_mode')
        self.set_content_of_text_field_by_id(shared_key, 'pre_shared_key')


    def set_visibility(self, visible=True):
        self.add_item_to_command_list(self._set_visibility, (visible,), 1, 900)


    def _set_visibility(self, visible=True):
        if visible:
            xpath = '//input[@value="1" and @name="ssid_broadcast"]'
        else:
            xpath = '//input[@value="0" and @name="ssid_broadcast"]'
        self.click_button_by_xpath(xpath)
