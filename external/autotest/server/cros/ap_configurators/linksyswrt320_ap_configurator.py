# Copyright (c) 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import urlparse

import linksyse2000_ap_configurator


class LinksysWRT320APConfigurator(
        linksyse2000_ap_configurator.Linksyse2000APConfigurator):
    """Base class for objects to configure Linksys WRT 320 access points
       using webdriver."""

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
