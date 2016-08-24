# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""FAFT config setting overrides for Skate."""


class Values(object):
    """FAFT config values for Skate."""
    software_sync_update = 6
    chrome_ec = True
    use_u_boot = True
    ec_capability = (['battery', 'keyboard', 'arm', 'lid'])
    has_eventlog = False        # No RTC support in firmware
