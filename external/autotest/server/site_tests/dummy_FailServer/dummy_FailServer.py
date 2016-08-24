# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.server import test
from autotest_lib.client.common_lib import error

class dummy_FailServer(test.test):
    """A test that always fails."""
    version = 1

    def run_once(self):
        """Run the test that always fails, once"""
        raise error.TestFail('Test always fails intentionally.')