# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


"""Module containing the constants to be reused throughout suite_scheduler."""


class Labels:
    """Constants related to label names.

    @var BOARD_PREFIX The string with which board labels are prefixed.
    @var POOL_PREFIX The stright with which pool labels are prefixed."""
    BOARD_PREFIX = 'board:'
    POOL_PREFIX = 'pool:'


class Builds:
    """Constants related to build type.

    @var FIRMWARE_RW: The string indicating the given build is used to update
                      RW firmware.
    @var CROS: The string indicating the given build is used to update ChromeOS.
    """
    FIRMWARE_RW = 'firmware_rw'
    CROS = 'cros'