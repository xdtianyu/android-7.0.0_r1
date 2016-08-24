# Copyright (c) 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Class to control the Dlink DWL2100 router."""

import logging
import time

import ap_spec
import dynamic_ap_configurator

class DLinkDWL2100APConfigurator(
        dynamic_ap_configurator.DynamicAPConfigurator):
    """Derived class to conrol DLink DWL2100 AP."""

    def get_number_of_pages(self):
        return 1


    def get_supported_bands(self):
        """Returns a list of dictionaries describing the supported bands.

        Example: returned is a dictionary of band and a list of channels. The
                 band object returned must be one of those defined in the
                 __init___ of this class.

        supported_bands = [{'band' : self.band_2GHz,
                            'channels' : [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11]},
                           {'band' : ap_spec.BAND_5GHZ,
                            'channels' : [26, 40, 44, 48, 149, 153, 165]}]

        Note: The derived class must implement this method.

        @return a list of dictionaries as described above

        """
        return [{'band': ap_spec.BAND_2GHZ,
                 'channels': [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11]}]


    def get_supported_modes(self):
        """
        Returns a list of dictionaries describing the supported modes.

        Example: returned is a dictionary of band and a list of modes. The band
                 and modes objects returned must be one of those defined in the
                 __init___ of this class.

        supported_modes = [{'band' : ap_spec.BAND_2GHZ,
                            'modes' : [mode_b, mode_b | mode_g]},
                           {'band' : ap_spec.BAND_5GHZ,
                            'modes' : [mode_a, mode_n, mode_a | mode_n]}]

        Note: The derived class must implement this method.

        @return a list of dictionaries as described above

        """
        return [{'band': ap_spec.BAND_2GHZ,
                 'modes': [ap_spec.MODE_G]}]


    def is_security_mode_supported(self, security_mode):
        """
        Returns if a given security_type is supported.

        Note: The derived class must implement this method.

        @param security_mode: one of the following modes:
                         self.security_disabled,
                         self.security_wep,
                         self.security_wpapsk,
                         self.security_wpa2psk

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

        Note: The derived class must implement this method.

        @param page_number: page number of the page to load

        """
        page_url = self.admin_interface_url
        self.get_url(page_url, page_title='DWL-2100AP')
        wireless_xpath = ("//html/body/table/tbody/tr[2]/td/table/tbody/tr[3]"
                          "/td[1]/table/tbody/tr[2]/td[@class='style1']")
        self.click_button_by_xpath(wireless_xpath)
        # We wait for the page to load and avoid the intermediate page
        found_id = self.wait_for_objects_by_id(['AuthMenu'],
                                               wait_time=30)
        if 'AuthMenu' not in found_id:
            raise RuntimeError(
                    'Unable to navigate to configuration page.')


    def save_page(self, page_number):
        """
        Saves the given page.

        Note: The derived class must implement this method.

        @param page_number: Page number of the page to save.

        """
        self.driver.execute_script('formSubmit(0);')
        count_time = '//input[@name="V_Count_Time"]'
        timer = self.wait_for_object_by_xpath(count_time)
        while (self.wait_for_object_by_xpath(count_time) and
               int(timer.get_attribute('value')) > 2):
            time.sleep(1)


    def set_mode(self, mode, band=None):
        """
        Sets the mode.

        Note: The derived class must implement this method.

        @param mode: must be one of the modes listed in __init__()
        @param band: the band to select

        """
        logging.debug('This router (%s) does not support multiple modes.',
                      self.name)
        return None


    def set_radio(self, enabled=True):
        """
        Turns the radio on and off.

        Note: The derived class must implement this method.

        @param enabled: True to turn on the radio; False otherwise

        """
        logging.debug('This AP does not supported disabling the radio.')
        return None


    def set_ssid(self, ssid):
        """
        Sets the SSID of the wireless network.

        Note: The derived class must implement this method.

        @param ssid: name of the wireless network

        """
        self.add_item_to_command_list(self._set_ssid, (ssid,), 1, 900)


    def _set_ssid(self, ssid):
        self._ssid = ssid
        self.set_content_of_text_field_by_id(self._ssid, 'Ssid')


    def set_channel(self, channel):
        """
        Sets the channel of the wireless network.

        Note: The derived class must implement this method.

        @param channel: integer value of the channel

        """
        self.add_item_to_command_list(self._set_channel, (channel,), 1, 900)


    def _set_channel(self, channel):
        position = self._get_channel_popup_position(channel)
        channel_choices = ['1 ', '2', '3', '4', '5', '6', '7',
                           '8', '9', '10', '11']
        channel_popup = self.driver.find_element_by_id('Channel')
        self.select_item_from_popup_by_id(channel_choices[position],
                                          'Channel')


    def set_band(self, band):
        """
        Sets the band of the wireless network.

        Currently there are only two possible values for band: 2kGHz and 5kGHz.
        Note: The derived class must implement this method.

        @param band: Constant describing the band type

        """
        logging.debug('This router (%s) does not support multiple bands.',
                      self.name)
        return None


    def set_security_disabled(self):
        """
        Disables the security of the wireless network.

        Note: The derived class must implement this method.

        """
        self.add_item_to_command_list(self._set_security_disabled, (), 1, 900)


    def _set_security_disabled(self):
        self.select_item_from_popup_by_id('Open System', 'AuthMenu')
        self.click_button_by_id('DisableEncryption')


    def set_security_wep(self, key_value, authentication):
        """
        Enabled WEP security for the wireless network.

        Note: The derived class must implement this method.

        @param key_value: encryption key to use
        @param authentication: one of two supported WEP authentication types:
                               open or shared.
        """
        logging.debug('This router (%s) does not support WEP.',
                      self.name)
        return None


    def set_security_wpapsk(self, security, shared_key, update_interval=1800):
        """Enabled WPA using a private security key for the wireless network.

        Note: The derived class must implement this method.

        @param security: Required security for AP configuration
        @param shared_key: shared encryption key to use
        @param update_interval: number of seconds to wait before updating

        """
        self.add_item_to_command_list(self._set_security_wpapsk,
                                      (security, shared_key, update_interval),
                                       1, 900)


    def _set_security_wpapsk(self, security, shared_key, update_interval=1800):
        if security == ap_spec.SECURITY_TYPE_WPAPSK:
            self.select_item_from_popup_by_id('WPA-PSK', 'AuthMenu')
        if security == ap_spec.SECURITY_TYPE_WPA2PSK:
            self.select_item_from_popup_by_id('WPA2-PSK', 'AuthMenu')
        self.select_item_from_popup_by_id('AUTO', 'Cipher',
             wait_for_xpath='id("GKUI")')
        if update_interval in range(300, 9999999):
            self.set_content_of_text_field_by_id(str(update_interval),
                                             'GKUI')
        else:
            raise RuntimeError('Update interval is %, not within range',
                               update_interval)
        self.set_content_of_text_field_by_id(shared_key,
                                             'PassPhrase')


    def set_visibility(self, visible=True):
        """Set the visibility of the wireless network.

        Note: The derived class must implement this method.

        @param visible: True for visible; False otherwise

        """
        self.add_item_to_command_list(self._set_visibility, (visible,), 1, 900)


    def _set_visibility(self, visible=True):
        found_id = self.wait_for_objects_by_id(['SsidBroadcast'],
                                               wait_time=30)
        if ('SsidBroadcast' in found_id) and visible:
            self.select_item_from_popup_by_id('Enable', 'SsidBroadcast')
        else:
            self.select_item_from_popup_by_id('Disable', 'SsidBroadcast')
