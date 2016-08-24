# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""FAFT config setting overrides for Oak."""

class Values(object):
    """FAFT config values for Oak."""

    chrome_ec = True
    ec_capability = ['battery', 'charging', 'keyboard', 'arm', 'lid']