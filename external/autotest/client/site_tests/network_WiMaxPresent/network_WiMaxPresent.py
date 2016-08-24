# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.networking import wimax_proxy


class network_WiMaxPresent(test.test):
    """Verifies that a WiMAX module can connect to a WiMAX network.

       This test verifies that a build can support WiMAX properly. It needs to
       run on a DUT with a supported WiMAX module, but does not require a WiMAX
       network.  It simply checks if shill creates a WiMAX device object, which
       verifies that:
       - The kernel detects a WiMAX module, loads the WiMAX driver, and
         downloads the WiMAX firmware to the module.
       - The WiMAX manager detects the WiMAX module exposed by the kernel.
       - The WiMAX manager is running and can communicate with shill over DBus.
       - shill is built with WiMAX support.

    """
    version = 1


    def run_once(self, **kwargs):
        proxy = wimax_proxy.WiMaxProxy()
        device = proxy.find_wimax_device_object()
        if not device:
            raise error.TestError('Could not find a WiMAX device.')
