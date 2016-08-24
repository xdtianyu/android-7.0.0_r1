# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""FAFT config setting overrides for ZGB."""


class Values(object):
    """FAFT config values for ZGB."""
    mode_switcher_type = 'physical_button_switcher'
    gbb_version = 1.0
    need_dev_transition = True
    has_eventlog = False
