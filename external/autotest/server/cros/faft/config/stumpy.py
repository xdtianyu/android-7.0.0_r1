# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""FAFT config setting overrides for Stumpy."""


class Values(object):
    """FAFT config values for Stumpy."""
    mode_switcher_type = 'physical_button_switcher'
    has_lid = False
    has_keyboard = False
    has_eventlog = False
