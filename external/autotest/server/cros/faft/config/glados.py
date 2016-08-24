# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""FAFT config setting overrides for GLaDOS."""

class Values(object):
    """FAFT config values for Glados."""
    chrome_ec = True
    chrome_usbpd = True
    dark_resume_capable = True
    ec_capability = ['battery', 'charging', 'doubleboot',
                     'keyboard', 'lid', 'x86', 'usbpd_uart' ]
    wp_voltage = 'pp3300'
    spi_voltage = 'pp3300'
    firmware_screen = 16
    servo_prog_state_delay = 10
    usb_plug = 16
