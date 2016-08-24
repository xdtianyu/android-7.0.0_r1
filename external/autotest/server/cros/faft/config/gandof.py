# Copyright (c) 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""FAFT configuration overrides for Gandof."""

from autotest_lib.server.cros.faft.config import auron


class Values(auron.Values):
    """Inherit overrides from auron."""
    ec_capability = ['adc_ectemp', 'battery', 'charging',
                     'keyboard', 'lid', 'x86', 'usb', 'peci',
                     'smart_usb_charge']

