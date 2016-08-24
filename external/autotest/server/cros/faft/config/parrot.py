# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""FAFT config setting overrides for Parrot."""


class Values(object):
    """FAFT config values for Parrot."""
    # Parrot uses UART to switch to rec mode instead of gpio thus to
    # clear rec_mode, devices needs to be sufficiently booted.
    ec_boot_to_console = 4

    dark_resume_capable = True
    key_checker = [[0x29, 'press'],
                   [0x32, 'press'],
                   [0x32, 'release'],
                   [0x29, 'release'],
                   [0x47, 'press'],
                   [0x47, 'release']]
