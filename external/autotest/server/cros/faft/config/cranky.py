# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""FAFT configuration overrides for Cranky."""

from autotest_lib.server.cros.faft.config import rambi


class Values(rambi.Values):
    """Inherit overrides from rambi."""
    ec_capability = ['battery', 'charging', 'x86',
                     'usb', 'smart_usb_charge']
    has_lid = False
    has_keyboard = False
