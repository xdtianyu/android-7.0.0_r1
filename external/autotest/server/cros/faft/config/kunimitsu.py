# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""FAFT config setting overrides for Kunimitsu."""

class Values(object):
    """FAFT config values for Kunimitsu."""
    chrome_ec = True
    dark_resume_capable = True
    wp_voltage = 'pp3300'
    spi_voltage = 'pp3300'
    ec_boot_to_console = 0.2
    ec_capability = ['battery', 'charging', 'doubleboot', 'keyboard',
                     'lid', 'x86', 'usb', 'usbpd_uart' ]

