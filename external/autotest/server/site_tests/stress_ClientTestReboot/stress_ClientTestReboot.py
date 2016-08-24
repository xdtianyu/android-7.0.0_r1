# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.server import autotest
from autotest_lib.server import hosts
from autotest_lib.server import test


class stress_ClientTestReboot(test.test):
    """Reboot a device."""
    version = 1

    def run_once(self, client_ip, testname, loops):
        host = hosts.create_host(client_ip)
        autotest_client = autotest.Autotest(host)
        for i in xrange(loops):
            logging.debug('Starting loop #%d', i)
            autotest_client.run_test(testname)
            host.reboot()
