# Copyright (c) 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


import ap_spec
import belkinF9K_ap_configurator


class BelkinF5D8236APConfigurator(
        belkinF9K_ap_configurator.BelkinF9KAPConfigurator):
    """Class to configure Blekin f5d8236-4 v2 router."""

    def __init__(self, ap_config):
        super(BelkinF5D8236APConfigurator, self).__init__(ap_config)
        self._dhcp_delay = 0


    def _set_security_wpapsk(self, security, shared_key, update_interval=None):
        key_field = '//input[@name="wpa_key_pass"]'
        psk = '//select[@name="authentication"]'
        self.select_item_from_popup_by_xpath('WPA/WPA2-Personal (PSK)',
                                             self.security_popup,
                                             wait_for_xpath=key_field,
                                             alert_handler=self._security_alert)
        selection = 'WPA2-PSK'
        if security == ap_spec.SECURITY_TYPE_WPAPSK:
            selection = 'WPA-PSK'
        self.select_item_from_popup_by_xpath(selection, psk,
                                             wait_for_xpath=key_field,
                                             alert_handler=self._security_alert)
        self.set_content_of_text_field_by_xpath(shared_key, key_field,
                                                abort_check=False)
