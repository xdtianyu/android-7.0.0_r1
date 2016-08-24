# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.server import autotest
from autotest_lib.server import hosts
from autotest_lib.server import test

#TODO(chromium:500040) Do not assume eth0 as network interface to monitor.
class hardware_StorageQualTrimStress(test.test):
    """Do traffic and trim while suspending aggressively."""

    version = 1
    def run_once(self, client_ip, duration):
        client = hosts.create_host(client_ip)
        client_at = autotest.Autotest(client)
        control = """job.parallel(
            [lambda: job.run_test('power_SuspendStress', tag='disk',
                duration=%d, init_delay=10, min_suspend=7, min_resume=30,
                interface='eth0')],
            [lambda: job.run_test('hardware_TrimIntegrity', test_length=%d+30,
                disable_sysinfo=True,
                tag='qual_trim')])""" % (duration, duration-30)
        client_at.run(control, '.', None)

