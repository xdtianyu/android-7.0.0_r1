# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import cellular_system_error


class AirStateVerifierPermissive(object):
    """An abstraction for verifying the air-side cellular state.

    This version is for commercial networks where we can't verify
    anything, so it's a no-op."""
    def AssertDataStatusIn(self, states):
        """Assert that the device's status is in states.
        Arguments:
            states:  Collection of states
        Raises:
            Error on failure.
        """
        # This base class is for commercial networks.  It can't verify, so
        # it doesn't
        pass

    def IsDataStatusIn(self, expected):
        return True


class AirStateVerifierBasestation(object):
    """An abstraction for verifying the air-side cellular state.

    This version checks with the base station emulator.
    """
    def __init__(self, base_station):
        self.base_station = base_station

    def IsDataStatusIn(self, expected):
        actual = self.base_station.GetUeDataStatus()
        return actual in expected

    def AssertDataStatusIn(self, expected):
        actual = self.base_station.GetUeDataStatus()
        if actual not in expected:
            raise cellular_system_error.BadState(
                'expected UE in status %s, got %s' % (expected, actual))
