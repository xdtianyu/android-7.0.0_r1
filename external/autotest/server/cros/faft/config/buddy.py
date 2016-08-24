# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""FAFT configuration overrides for Buddy."""

from autotest_lib.server.cros.faft.config import auron


class Values(auron.Values):
    """Inherit overrides from auron."""
    ec_capability = ['adc_ectemp', 'x86', 'usb', 'peci']
    has_lid = False
    has_keyboard = False
    rec_button_dev_switch = True
