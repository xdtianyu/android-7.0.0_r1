# Copyright (c) 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import urlparse

import linksyse2100_ap_configurator


class LinksysWRT160APConfigurator(linksyse2100_ap_configurator.
                                   Linksyse2100APConfigurator):
    """Derived class to control Linksys WRT160Nv3 router."""

    def navigate_to_page(self, page_number):
        if page_number == 1:
            page_url = urlparse.urljoin(self.admin_interface_url,
                                        'Wireless_Basic.asp')
            self.get_url(page_url, page_title='Settngs')
        elif page_number == 2:
            page_url = urlparse.urljoin(self.admin_interface_url,
                                        'WL_WPATable.asp')
            self.get_url(page_url, page_title='Security')
        else:
            raise RuntimeError('Invalid page number passed. Number of pages '
                               '%d, page value sent was %d' %
                               (self.get_number_of_pages(), page_number))


    def _set_channel(self, channel):
        position = self._get_channel_popup_position(channel)
        xpath = '//select[@name="wl_schannel"]'
        channels = ['Auto', '1', '2', '3', '4', '5', '6', '7', '8',
                    '9', '10', '11']
        self.select_item_from_popup_by_xpath(channels[position], xpath)


    def _set_channel_width(self, channel_wid):
        channel_width_choice = ['Auto (20 MHz or 40 MHz)', '20MHz only']
        xpath = '//select[@name="_wl_nbw"]'
        self.select_item_from_popup_by_xpath(channel_width_choice[channel_wid],
                                             xpath)


    def _set_security_wpapsk(self, security, shared_key, update_interval=3600):
        if update_interval not in range(600, 7201):
           logging.info('The update interval should be between 600 and 7200.'
                       'Setting the interval to default (3600)')
           update_interval = 3600
        super(LinksysWRT160APConfigurator, self)._set_security_wpapsk(security,
                                          shared_key, update_interval)
        text = '//input[@name="wl_wpa_gtk_rekey"]'
        self.set_content_of_text_field_by_xpath(update_interval, text,
                                                abort_check=True)
