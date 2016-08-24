#!/usr/bin/python
#
# Copyright (c) 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import re

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error


class kernel_sysrq_info(test.test):
    """
    Verify the Magic SysRq show-* commands
    (i.e. don't verify reBoot, Crash, kill-all-tasks, etc.)
    """
    version = 1

    def sysrq_trigger(self, key):
        """
        Trigger one SysRq command, and return the kernel log output
        @param key:     lowercase SysRq keystroke (e.g. 'm')
        @return         dmesg log from running the command
        """
        os.system("dmesg --clear")
        with open("/proc/sysrq-trigger", "w") as f:
            f.write(key + "\n")
        with os.popen("dmesg --raw") as f:
            return f.read()

    def run_once(self):
        test_cases = {'l': 'all active CPUs',
                      'm': '[0-9]+ pages.*RAM',
                      'p': 'Show Regs',
                      'q': 'Tick Device:',
                      't': 'init.*\s1\s',
                      'w': 'pid father'
                     }

        for key in test_cases:
            s = self.sysrq_trigger(key)
            if re.search(test_cases[key], s) == None:
                raise error.TestFail('Unexpected output from SysRq key %s' %
                                     key)
