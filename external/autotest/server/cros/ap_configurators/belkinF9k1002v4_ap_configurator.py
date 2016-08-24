# Copyright (c) 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import ap_spec
import belkinF9K_ap_configurator


class BelkinF9K1002v4APConfigurator(belkinF9K_ap_configurator.
                                    BelkinF9KAPConfigurator):
    """Derived class to control the BelkinF9K 1002v4 AP configurator."""

    def __init__(self, ap_config):
        super(BelkinF9K1002v4APConfigurator, self).__init__(ap_config)
        self._dhcp_delay = 0


    def _set_security_wpapsk(self, security, shared_key, update_interval=None):
        security_popup = '//select[@name="security_type"]'
        key_field = '//input[@name="wpa_key_text"]'
        psk = '//select[@name="authentication"]'
        self.select_item_from_popup_by_xpath('WPA/WPA2-Personal (PSK)',
                                             self.security_popup,
                                             wait_for_xpath=key_field,
                                             alert_handler=self._security_alert)
        auth_type = 'WPA-PSK'
        if security == ap_spec.SECURITY_TYPE_WPA2PSK:
            auth_type = 'WPA2-PSK'
        self.select_item_from_popup_by_xpath(auth_type, psk,
                                             wait_for_xpath=key_field,
                                             alert_handler=self._security_alert)
        self.set_content_of_text_field_by_xpath(shared_key, key_field,
                                                abort_check=True)
