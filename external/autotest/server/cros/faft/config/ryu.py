# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""FAFT configuration overrides for Ryu."""


class Values(object):
    """FAFT config values for Ryu."""

    mode_switcher_type = 'ryu_switcher'
    fw_bypasser_type = 'ryu_bypasser'

    has_lid = False
    has_keyboard = False

    chrome_ec = True
    ec_capability = ['arm', 'battery', 'charging']
