# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""FAFT config setting overrides for Pi."""


class Values(object):
    """FAFT config values for Pi."""
    chrome_ec = True
    ec_capability = (['battery', 'keyboard', 'arm', 'lid'])
    has_eventlog = False        # No RTC support in firmware
