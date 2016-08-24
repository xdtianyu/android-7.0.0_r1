# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import signal

from autotest_lib.server.cros.network import hostap_config
from autotest_lib.server.cros.network import wifi_cell_test_base

class network_WiFi_Manual(wifi_cell_test_base.WiFiCellTestBase):
    """Set up an AP, so that we can test things manually."""

    version = 1


    def run_once(self):
        """Body of the test."""
        self.context.configure(hostap_config.HostapConfig(
            channel=1, ssid='manual_test',
            mode=hostap_config.HostapConfig.MODE_11N_MIXED,
            n_capabilities=
            [hostap_config.HostapConfig.N_CAPABILITY_HT40_PLUS]))
        signal.pause()
