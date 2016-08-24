# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.client.common_lib import error
from autotest_lib.server import autotest
from autotest_lib.server import hosts
from autotest_lib.server import test


class bluetooth_RegressionServer(test.test):
    """Server part of the Bluetooth Semi-Automatic Regression Tests."""
    version = 1

    def run_once(self, client_ip, device_addrs):
        """Run Server side of Bluetooth Regression tests.

        @param client_ip: Device under test.
        @param device_addrs: MAC addresses of Bluetooth devices under test.
        """
        if not client_ip:
            error.TestError('Must provide client\'s IP address to test')

        client = hosts.create_host(client_ip)
        client_at = autotest.Autotest(client)

        logging.info('Running client side tests')
        client_at.run_test('bluetooth_RegressionClient',
                            addrs=device_addrs, close_browser=False,
                            test_phase='reboot')
        logging.info('Starting reboot from Server')
        client.reboot()
        logging.info('Returning to Client after reboot')
        client_at.run_test('bluetooth_RegressionClient',
                           addrs=device_addrs, test_phase='client')
