# Copyright (c) 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""FAFT config overrides for Brain (RK3288 base platform without EC)."""

from autotest_lib.server.cros.faft.config import veyron

class Values(veyron.Values):
    """Inherit overrides from Veyron."""
    has_lid = False
    has_keyboard = False
    rec_button_dev_switch = True
    hold_pwr_button_poweroff = 7
    hold_pwr_button_poweron = 0.7
    powerup_ready = 15
