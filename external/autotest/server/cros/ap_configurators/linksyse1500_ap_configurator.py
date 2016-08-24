# Copyright (c) 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import ap_spec
import linksyse2100_ap_configurator


class Linksyse1500APConfigurator(linksyse2100_ap_configurator.
                                 Linksyse2100APConfigurator):
    """Derived class to control Linksys E1500 router."""

    def _set_mode(self, mode, band=None):
        mode_mapping = {ap_spec.MODE_M:'Mixed',
                        ap_spec.MODE_B | ap_spec.MODE_G:'Wireless-B/G Only',
                        ap_spec.MODE_G:'Wireless-G Only',
                        ap_spec.MODE_B:'Wireless-B Only',
                        ap_spec.MODE_N:'Wireless-N Only',
                        'Disabled':'Disabled'}
        mode_name = mode_mapping.get(mode)
        if not mode_name:
            raise RuntimeError('The mode %d not supported by router %s. ',
                               hex(mode), self.name)
        xpath = '//select[@name="wl_net_mode"]'
        self.select_item_from_popup_by_xpath(mode_name, xpath,
                                             alert_handler=self._sec_alert)


    def _set_channel(self, channel):
        position = self._get_channel_popup_position(channel)
        xpath = '//select[@name="wl_schannel"]'
        channels = ['Auto', '1', '2', '3', '4', '5', '6', '7', '8',
                    '9', '10', '11']
        self.select_item_from_popup_by_xpath(channels[position], xpath)
