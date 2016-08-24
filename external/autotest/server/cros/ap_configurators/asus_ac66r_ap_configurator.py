# Copyright (c) 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Class to control the Asus66RAP router."""

import ap_spec
import asus_qis_ap_configurator


class Asus66RAPConfigurator(asus_qis_ap_configurator.AsusQISAPConfigurator):
    """Derives class for Asus RT-AC66R."""

    def set_mode(self, mode, band=None):
        self.set_security_disabled() #  To avoid the modal dialog.
        self.add_item_to_command_list(self._set_mode, (mode, band), 1, 900)


    def _set_mode(self, mode, band=None):
        if band:
            self._set_band(band)
        if mode == ap_spec.MODE_AUTO:
            mode_popup = 'Auto'
        elif mode == ap_spec.MODE_N:
            mode_popup = 'N Only'
        else:
            raise RuntimeError('Invalid mode passed %x' % mode)
        xpath = '//select[@name="wl_nmode_x"]'
        self.select_item_from_popup_by_xpath(mode_popup, xpath,
              alert_handler=self._invalid_security_handler)


    def set_channel(self, channel):
        self.add_item_to_command_list(self._set_channel, (channel,), 1, 900)


    def _set_channel(self, channel):
        position = self._get_channel_popup_position(channel)
        channel_choices = ['Auto', '1', '2', '3', '4', '5', '6',
                           '7', '8', '9', '10', '11']
        xpath = '//select[@name="wl_chanspec"]'
        if self.current_band == ap_spec.BAND_5GHZ:
            channel_choices = ['Auto', '36', '40', '44', '48', '149', '153',
                               '157', '161']
        self.select_item_from_popup_by_xpath(str(channel_choices[position]),
                                             xpath)
