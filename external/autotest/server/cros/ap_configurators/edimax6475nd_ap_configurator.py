# Copyright (c) 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import ap_spec
import edimax_ap_configurator


class Edimax6475ndAPConfigurator(
        edimax_ap_configurator.EdimaxAPConfigurator):
    """Derived class to control the Edimax BR-6475ND AP."""


    def navigate_to_page(self, page_number):
        """Navigate to the required page.

        @param page_number: The page number to navigate to.

        """
        if page_number != 1 and page_number != 2:
            raise RuntimeError('Invalid page number passed.  Number of pages is'
                               '%d, page value sent was %d' %
                               (self.get_number_of_pages(), page_number))
        page_url = self.admin_interface_url
        self.get_url(page_url, page_title='EDIMAX Technology')
        frame = self.driver.find_element_by_xpath('//frame[@name="mainFrame"]')
        self.driver.switch_to_frame(frame)
        main_tabs = self.driver.find_elements_by_css_selector('div')
        main_tabs[2].click()
        sub_tabs = self.driver.find_elements_by_xpath(
                                                     '//span[@class="style11"]')
        if self.current_band == ap_spec.BAND_2GHZ:
            sub_tabs[2].click()
        else:
            sub_tabs[3].click()
        if page_number == 1:
            # Open the general settings page.
            self.click_button_by_xpath('//input[@onclick="c_fun(0)" and
                                        @name="sys"]')
            self.wait_for_object_by_xpath('//select[@name="band"]')
        else:
            # Open the security settings page.
            self.click_button_by_xpath('//input[@onclick="c_fun(1)" and
                                       @name="sys"]')
            self.wait_for_object_by_xpath('//select[@name="method"]')


    def get_supported_bands(self):
        return [{'band': ap_spec.BAND_2GHZ, 'channels': range(1, 11)},
                {'band': ap_spec.BAND_5GHZ,
                 'channels': [36, 40, 44, 48, 52, 56, 60, 64, 100, 104, 108,
                              136, 140, 149, 153, 157, 161, 165]}]


    def set_band(self, band):
        if band == ap_spec.BAND_5GHZ:
            self.current_band = ap_spec.BAND_5GHZ
        elif band == ap_spec.BAND_2GHZ:
            self.current_band = ap_spec.BAND_2GHZ
        else:
            raise RuntimeError('Invalid band sent %s' % band)


    def get_supported_modes(self):
        return [{'band': ap_spec.BAND_2GHZ,
                 'modes': [ap_spec.MODE_B,
                           ap_spec.MODE_G,
                           ap_spec.MODE_N,
                           ap_spec.MODE_B | ap_spec.MODE_G,
                           ap_spec.MODE_B | ap_spec.MODE_G | ap_spec.MODE_N]},
                {'band': ap_spec.BAND_5GHZ,
                 'modes': [ap_spec.MODE_A,
                           ap_spec.MODE_N,
                           ap_spec.MODE_A | ap_spec.MODE_N]}]


    def _set_mode(self, mode, band=None):
        # Create the mode to popup item mapping
        mode_mapping_2ghz = {ap_spec.MODE_B | ap_spec.MODE_G | ap_spec.MODE_N:
                             '2.4 GHz (B+G+N)',
                             ap_spec.MODE_N: '2.4 GHz (N)',
                             ap_spec.MODE_B: '2.4 GHz (B)',
                             ap_spec.MODE_G: '2.4 GHz (G)',
                             ap_spec.MODE_B | ap_spec.MODE_G: '2.4 GHz (B+G)'}
        mode_mapping_5ghz = {ap_spec.MODE_A: '5 GHz (A)',
                             ap_spec.MODE_N: '5 GHz (N)',
                             ap_spec.MODE_A | ap_spec.MODE_N: '5 GHz (A+N)'}
        mode_name = ''
        if mode in mode_mapping_2ghz.keys() or mode in mode_mapping_5ghz.keys():
            if self.current_band == ap_spec.BAND_2GHZ:
                mode_name = mode_mapping_2ghz[mode]
            else:
                mode_name = mode_mapping_5ghz[mode]
        else:
            raise RuntimeError('The mode selected %d is not supported by router'
                               ' %s.', hex(mode), self.name)
        xpath = '//select[@name="band"]'
        self.select_item_from_popup_by_xpath(mode_name, xpath)


    def _set_channel(self, channel):
        position = self._get_channel_popup_position(channel)
        channel_choices_2ghz = ['1', '2', '3', '4', '5',
                                '6', '7', '8', '9', '10', '11']
        channel_choices_5ghz = [ '36', '40', '44', '48', '52', '56', '60',
                                 '64', '100', '104', '108', '136', '140',
                                 '149', '153', '157', '161', '165']
        if self.current_band == ap_spec.BAND_2GHZ:
            self.select_item_from_popup_by_xpath(channel_choices_2ghz[position],
                                                 '//select[@name="chan"]')
        else:
            self.select_item_from_popup_by_xpath(channel_choices_5ghz[position],
                                                 '//select[@name="chan"]')
