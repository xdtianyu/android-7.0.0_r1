# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""FAFT config setting overrides for kitty."""

from autotest_lib.server.cros.faft.config import nyan

class Values(nyan.Values):
    ec_capability = ['arm']
    firmware_screen = 7
    has_lid = False
    has_keyboard = False
    rec_button_dev_switch = True
