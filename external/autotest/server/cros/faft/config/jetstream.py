# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""FAFT configuration overrides for JetStream."""


class Values(object):
    """FAFT config values for JetStream."""

    mode_switcher_type = 'jetstream_switcher'
    fw_bypasser_type = 'jetstream_bypasser'

    has_lid = False
    has_keyboard = False
    rec_button_dev_switch = True
