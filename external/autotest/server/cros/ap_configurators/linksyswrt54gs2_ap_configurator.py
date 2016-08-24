# Copyright (c) 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import ap_spec
import linksys_ap_configurator


class LinksysWRT54GS2APConfigurator(
        linksys_ap_configurator.LinksysAPConfigurator):
    """Derived class to control Linksys WRT54GS2 router."""

    def get_supported_modes(self):
        return [{'band': ap_spec.BAND_2GHZ,
                 'modes': [ap_spec.MODE_B, ap_spec.MODE_G, ap_spec.MODE_M]}]


    def _set_mode(self, mode):
        mode_mapping = {ap_spec.MODE_B: 'B-Only', ap_spec.MODE_G: 'G-Only',
                        ap_spec.MODE_M: 'Mixed', 'Disabled' : 'Disabled'}
        mode_name = None
        if mode in self.get_supported_modes()[0]['modes']:
            mode_name = mode_mapping.get(mode)
        else:
            mode_name = mode
        if not mode_name:
            raise RuntimeError('The mode selected %d is not supported by router'
                               ' %s.', hex(mode), self.name)
        xpath = ('//select[@name="wl_net_mode"]')
        self.select_item_from_popup_by_xpath(mode_name, xpath)
