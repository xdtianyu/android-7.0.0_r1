# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.cellular.pseudomodem import pm_constants
from autotest_lib.client.cros.cellular.pseudomodem import pseudomodem_context
from autotest_lib.client.cros.networking.chrome_testing \
        import chrome_networking_test_context as cntc

class network_ChromeCellularNetworkPresent(test.test):
    """
    This test is meant as a simple example using
    client/cros/networking/chrome_testing. It uses telemetry and pseudomodem to
    setup a fake network and verify that it properly propagates to Chrome.

    """
    version = 1

    def run_once(self, family):
        with pseudomodem_context.PseudoModemManagerContext(
                True,
                {'family' : family}):
            with cntc.ChromeNetworkingTestContext() as test_context:
                networks = test_context.find_cellular_networks()
                if len(networks) != 1:
                    raise error.TestFail(
                            'Expected 1 cellular network, found ' +
                            str(len(networks)))

                network = networks[0]
                if network["Type"] != test_context.CHROME_NETWORK_TYPE_CELLULAR:
                    raise error.TestFail(
                            'Expected network of type "Cellular", found ' +
                            network["Type"])

                if not network["Name"].startswith(
                        pm_constants.DEFAULT_TEST_NETWORK_PREFIX):
                    raise error.TestFail('Network name is incorrect: ' +
                                         network["Name"])
