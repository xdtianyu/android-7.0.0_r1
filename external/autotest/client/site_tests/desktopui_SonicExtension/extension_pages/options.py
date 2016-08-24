# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""The extensions option page."""

from extension_pages import ExtensionPages
from selenium.webdriver.common.action_chains import ActionChains


CHECK_BOX_LIST = {'send_usage': 'sendUsage'}
TEXT_BOX_LIST = {'min_video_bitrate': 'minVideoBitrate',
                 'max_video_bitrate': 'maxVideoBitrate',
                 'max_tab_frame_rate': 'maxFrameRate',
                 'add_debug_dongle_ip': 'newReceiverIp'}
RADIO_BUTTON_LIST = {'tab_casting_quality':
                         {'extreme': 'ql-highest',
                          'high': 'ql-high',
                          'standard': 'ql-low',
                          'auto': 'ql-auto'},
                     'resolution':
                         {'854x480': '854x480',
                          '1280x720': '1280x720',
                          '1920x1080': '1920x1080'}
                    }
BUTTON_LIST = {'reload': 'reload'}


class OptionsPage(ExtensionPages):
    """Contains all the controls on the options page of the extension."""

    def __init__(self, driver, extension_id):
        """Constructor."""
        self._options_url = 'chrome-extension://%s/options.html' % extension_id
        ExtensionPages.__init__(self, driver, self._options_url)


    def set_value(self, field, value):
        """Set the value of a specific field on the options page.
        @param field: The name of the control
        @param value: The value to set
        """
        self.go_to_page()
        if field in CHECK_BOX_LIST.keys():
            check_box = self._get_check_box(CHECK_BOX_LIST[field], field)
            check_box.set_value(value)
        elif field in TEXT_BOX_LIST.keys():
            text_box = self._get_text_box(TEXT_BOX_LIST[field], field)
            text_box.set_value(value)
        elif field in RADIO_BUTTON_LIST.keys():
            radio_button = self._get_radio_button(
                RADIO_BUTTON_LIST[field][value], field)
            radio_button.click()


    def min_video_bitrate_text_box(self):
        """The minimum video bit rate text box."""
        return self._get_text_box(TEXT_BOX_LIST['min_video_bitrate'],
                                  'min_video_bitrate')


    def max_tab_frame_rate_text_box(self):
        """The maximum tab frame rate text box."""
        return self._get_text_box(TEXT_BOX_LIST['max_tab_frame_rate'],
                                  'max_tab_frame_rate')


    def reload_button(self):
        """The reload button."""
        return self._get_button(BUTTON_LIST['reload'], 'reload')


    def highest_projection_radio_button(self):
        """The Extreme tab projection quality radio button."""
        return self._get_radio_button(
                RADIO_BUTTON_LIST['highest_tab_projection_quality'],
                'highest_tab_projection_quality')


    def high_projection_radio_button(self):
        """The High tab projection quality radio button."""
        return self._get_radio_button(
                RADIO_BUTTON_LIST['high_tab_projection_quality'],
                'high_tab_projection_quality')


    def low_projection_radio_button(self):
        """The Low tab projection quality radio button."""
        return self._get_radio_button(
                RADIO_BUTTON_LIST['low_tab_projection_quality'],
                'low_tab_projection_quality')


    def resolution_640x360_radio_button(self):
        """The 640x360 resolution radio button."""
        return self._get_radio_button(RADIO_BUTTON_LIST['resolution_640_360'],
                                      'resolution_640_360')


    def resolution_854x480_radio_button(self):
        """The 854x640 resolution radio button."""
        return self._get_radio_button(RADIO_BUTTON_LIST['resolution_854_480'],
                                      'resolution_854_480')


    def resolution_1280x720_radio_button(self):
        """The 1280x720 resolution radio button."""
        return self._get_radio_button(RADIO_BUTTON_LIST['resolution_1280_720'],
                                      'resolution_1280_720')


    def resolution_1980x1080_radio_button(self):
        """The 1980x1080 resolution radio button."""
        return self._get_radio_button(
                RADIO_BUTTON_LIST['resolution_1920_1080'],
                'resolution_1920_1080')


    def tab_casting_standard_radio_button(self):
        """The tab casting mode standard radio button."""
        return self._get_radio_button(RADIO_BUTTON_LIST['cast_streaming_off'],
                                      'cast_streaming_off')


    def tab_casting_enchanced_radio_button(self):
        """The tab casting mode enhanced radio button."""
        return self._GetRadioButton(RADIO_BUTTON_LIST['cast_streaming_on'],
                                    'cast_streaming_on')


    def send_usage_check_box(self):
      """The send usage check box."""
      return self._get_check_box(CHECK_BOX_LIST['send_usage'], 'send_usage')


    def open_hidden_options_menu(self):
        """Open the hidden options page."""
        self.go_to_page()
        element = self._driver.find_element_by_id('cast-icon')
        double_click_action = ActionChains(self._driver).double_click(element)
        double_click_action.perform()
        double_click_action.perform()
