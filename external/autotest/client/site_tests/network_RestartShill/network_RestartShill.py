# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os

from autotest_lib.client.bin import test
from autotest_lib.client.cros.networking import shill_context
from autotest_lib.client.cros.networking import shill_proxy

class network_RestartShill(test.test):
    """
    Stop shill, restart it, check that we can talk to it.
    """
    DEFAULT_PROFILE_PATH = '/var/cache/shill/default.profile'
    version = 1


    def run_once(self, remove_profile):
        """Test main loop."""
        with shill_context.stopped_shill():
            if remove_profile:
                os.remove(self.DEFAULT_PROFILE_PATH)
        shill = shill_proxy.ShillProxy.get_proxy()
