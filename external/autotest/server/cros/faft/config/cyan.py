# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""FAFT config setting overrides for Cyan."""


class Values(object):
    """FAFT config values for Cyan."""
    chrome_ec = True
    ec_boot_to_console = 0.3
    ec_capability = ['battery', 'charging', 'keyboard', 'lid', 'x86',
                     'usb', 'smart_usb_charge']
    firmware_screen = 28
    usb_plug = 28
    long_rec_combo = True
    wp_voltage = 'pp1800'
    spi_voltage = 'pp1800'
