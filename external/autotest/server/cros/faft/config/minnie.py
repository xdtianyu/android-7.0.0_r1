# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""FAFT config setting overrides for Minnie."""

from autotest_lib.server.cros.faft.config import pinky

class Values(pinky.Values):
    """Inherit overrides from Pinky."""
    pass
