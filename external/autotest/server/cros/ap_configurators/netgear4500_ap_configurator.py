# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Base class for NetgearWNDR dual band routers."""

import netgear_WNDR_dual_band_configurator
import ap_spec


class Netgear4500APConfigurator(
        netgear_WNDR_dual_band_configurator.NetgearDualBandAPConfigurator):
    """Base class for Netgear WNDR 4500 dual band routers."""


    def _set_mode(self, mode, band=None):
        if mode == ap_spec.MODE_G or mode == ap_spec.MODE_A:
            mode = 'Up to 54 Mbps'
        elif mode == ap_spec.MODE_N:
            mode = 'Up to 450 Mbps'
        else:
            raise RuntimeError('Unsupported mode passed.')
        xpath = '//select[@name="opmode"]'
        if self.current_band == ap_spec.BAND_5GHZ:
            xpath = '//select[@name="opmode_an"]'
        self.wait_for_object_by_xpath(xpath)
        self.select_item_from_popup_by_xpath(mode, xpath)

