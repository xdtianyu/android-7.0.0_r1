# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""FAFT config setting overrides for Spring."""


class Values(object):
    """FAFT config values for Spring."""
    software_sync_update = 6
    chrome_ec = True
    use_u_boot = True
    ec_capability = (['battery', 'keyboard', 'arm', 'lid'])
    has_eventlog = False        # No RTC support in firmware
