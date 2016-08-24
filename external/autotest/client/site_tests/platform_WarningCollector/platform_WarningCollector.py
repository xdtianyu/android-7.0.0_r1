# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
from autotest_lib.client.bin import test, utils

class platform_WarningCollector(test.test):
    "Tests the warning collector daemon"
    version = 1

    def run_once(self):
        "Runs the test once"

        # Restart the warning collector daemon, trigger a test warning, and
        # verify that a kwarn file is created.
        utils.system("stop warn-collector")
        utils.system("rm -rf /var/run/kwarn")
        utils.system("start warn-collector")
        utils.system("sleep 0.1")
        lkdtm = "/sys/kernel/debug/provoke-crash/DIRECT"
        if os.path.exists(lkdtm):
            utils.system("echo WARNING > %s" % (lkdtm))
        else:
            utils.system("echo warning > /proc/breakme")
        utils.system("sleep 0.1")
        utils.system("test -f /var/run/kwarn/warning")
