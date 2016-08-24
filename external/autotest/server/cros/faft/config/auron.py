# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""FAFT configuration overrides for Auron."""


class Values(object):
    """FAFT config values for Auron."""
    ec_boot_to_console = 0.6
    chrome_ec = True
    dark_resume_capable = True
    ec_capability = ['adc_ectemp', 'battery', 'charging',
                     'keyboard', 'lid', 'x86', 'usb', 'peci']
    wp_voltage = 'pp3300'
    spi_voltage = 'pp3300'
