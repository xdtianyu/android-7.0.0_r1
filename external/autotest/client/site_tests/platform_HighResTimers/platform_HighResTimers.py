# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import re
from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error, utils

class platform_HighResTimers(test.test):
    version = 1

    def check_timers(self):
        timer_list = open('/proc/timer_list')
        for line in timer_list.readlines():
            match = re.search('^\s*\.resolution:\s(\d+)\s*nsecs$', line)
            if match:
                res = int(match.group(1))
                if (res != 1):
                    raise error.TestError('Timer resolution %d != 1 ns' % res)

    def run_once(self):
        try:
            self.check_timers()
        except error.TestError, e:
            raise error.TestFail(e)
