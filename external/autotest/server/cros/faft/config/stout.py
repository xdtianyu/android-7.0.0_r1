# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""FAFT configuration overrides for Stout."""


class Values():
    """FAFT config values for Stout."""
    key_checker = [[0x29, 'press'],
                   [0x32, 'press'],
                   [0x32, 'release'],
                   [0x29, 'release'],
                   [0x43, 'press'],
                   [0x43, 'release']]
