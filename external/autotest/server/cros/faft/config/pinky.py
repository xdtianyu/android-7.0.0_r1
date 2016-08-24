# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""FAFT config overrides for Pinky (RK3288 base platform with EC)."""

from autotest_lib.server.cros.faft.config import veyron

class Values(veyron.Values):
    """Inherit overrides from Veyron."""
    chrome_ec = True
    ec_capability = ['battery', 'charging', 'keyboard', 'arm', 'lid']
    ec_boot_to_console = 1.1
    software_sync_update = 6
