# Copyright (c) 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import netgear_single_band_configurator
from netgear_single_band_configurator import *

import ap_spec

class Netgear1000APConfigurator(netgear_single_band_configurator.
                                NetgearSingleBandAPConfigurator):
    """Derived class to control Netgear WNR1000v3 router."""


    def set_mode(self, mode):
        # The mode popup changes based on the security mode.  Set to no
        # security to get the right popup.
        self.add_item_to_command_list(self._set_security_disabled, (), 1, 799)
        self.add_item_to_command_list(self._set_mode, (mode, ), 1, 800)


    def _set_mode(self, mode):
        if mode == ap_spec.MODE_G:
            mode = 'Up to 54 Mbps'
        elif mode == ap_spec.MODE_N:
            mode = 'Up to 150 Mbps'
        else:
            raise RuntimeError('Unsupported mode passed.')
        xpath = '//select[@name="opmode"]'
        self.select_item_from_popup_by_xpath(mode, xpath)
