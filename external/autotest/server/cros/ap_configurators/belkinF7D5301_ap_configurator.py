# Copyright (c) 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import ap_spec
import belkinF9K_ap_configurator


class BelkinF7D5301APConfigurator(
        belkinF9K_ap_configurator.BelkinF9KAPConfigurator):
    """Class to configure Belkin F7D5301 router."""

    def __init__(self, ap_config):
        super(BelkinF7D5301APConfigurator, self).__init__(ap_config)
        self._dhcp_delay = 0


    def _set_mode(self, mode):
        mode_mapping = {ap_spec.MODE_G: '802.11g',
                        ap_spec.MODE_N: '1x1 802.11n',
                        ap_spec.MODE_B | ap_spec.MODE_G | ap_spec.MODE_N:
                        '802.11b & 802.11g & 1x1 802.11n'}
        mode_name = mode_mapping.get(mode)
        if not mode_name:
            raise RuntimeError('The mode %d not supported by router %s. ',
                               hex(mode), self.name)
        xpath = '//select[@name="wbr"]'
        self.select_item_from_popup_by_xpath(mode_name, xpath,
                                             wait_for_xpath=None,
                                             alert_handler=self._security_alert)

