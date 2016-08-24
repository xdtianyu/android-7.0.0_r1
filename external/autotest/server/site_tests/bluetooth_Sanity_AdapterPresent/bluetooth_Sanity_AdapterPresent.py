# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.common_lib import error
from autotest_lib.server.cros.bluetooth import bluetooth_test


class bluetooth_Sanity_AdapterPresent(bluetooth_test.BluetoothTest):
    """
    Verify that the client has a Bluetooth adapter.
    """
    version = 1

    def run_once(self):
        # Reset the adapter (if any) to the powered off state.
        if not self.device.reset_off():
            raise error.TestFail('DUT could not be reset to initial state')

        # Verify that there is an adapter. This will only return True if both
        # the kernel and bluetooth daemon see the adapter.
        if not self.device.has_adapter():
            raise error.TestFail('Adapter not present')
