# Copyright (c) 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


import ap_spec
import belkinF9K_ap_configurator


class BelkinF7DAPConfigurator(
        belkinF9K_ap_configurator.BelkinF9KAPConfigurator):
    """Class to configure Belkin F7D1301 v1 (01A) router."""

    def __init__(self, ap_config):
        super(BelkinF7DAPConfigurator, self).__init__(ap_config)
        self._dhcp_delay = 0


    def set_mode(self, mode):
        self.add_item_to_command_list(self._set_mode, (mode,), 1, 900)


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


    def _set_security_wpapsk(self, security, shared_key, update_interval=None):
        key_field = '//input[@name="wpa_key_pass"]'
        psk = '//select[@name="authentication"]'
        self.select_item_from_popup_by_xpath('WPA/WPA2-Personal (PSK)',
                                             self.security_popup,
                                             wait_for_xpath=key_field,
                                             alert_handler=self._security_alert)
        auth_type = 'WPA2-PSK'
        if security == ap_spec.SECURITY_TYPE_WPAPSK:
            auth_type = 'WPA-PSK'
        self.select_item_from_popup_by_xpath(auth_type, psk,
                                             alert_handler=self._security_alert)
        self.set_content_of_text_field_by_xpath(shared_key, key_field,
                                                abort_check=False)
