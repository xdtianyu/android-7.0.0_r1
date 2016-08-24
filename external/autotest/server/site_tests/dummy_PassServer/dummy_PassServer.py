# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.server import test

class dummy_PassServer(test.test):
    """Tests that server tests can pass."""
    version = 1

    def run_once(self):
        """There is no body for this test."""
        return
