# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""FAFT configuration overrides for Gnawty."""

from autotest_lib.server.cros.faft.config import rambi


class Values(rambi.Values):
    """Inherit overrides from rambi."""
    wp_voltage = 'pp3300'
    spi_voltage = 'pp3300'
