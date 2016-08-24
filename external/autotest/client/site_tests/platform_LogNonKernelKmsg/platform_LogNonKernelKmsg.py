# Copyright 2014 The Chromium OS Authors. All rights reserved.  Use of
# this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import time

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import utils

class platform_LogNonKernelKmsg(test.test):
    """Test that we log non-kernel messages via /dev/kmsg"""
    KLOG_PATH = '/dev/kmsg'
    SYSLOG_BIN = 'rsyslogd'
    SYSLOG_JOB = 'syslog'
    SYSTEM_LOG_PATH = '/var/log/messages'
    version = 1


    def run_once(self):
        utils.run('truncate -s 0 %s' % self.SYSTEM_LOG_PATH)
        our_message = 'non-kernel message generated at %d\n' % time.time()
        with open(self.KLOG_PATH, 'w') as outfile:
            outfile.write(our_message)

        cmd_result = utils.run(
            'grep "%s" %s' % (our_message, self.SYSTEM_LOG_PATH),
            ignore_status=True)
        if cmd_result.exit_status:
            raise error.TestFail(
                'our log message did not appear in %s' % self.SYSTEM_LOG_PATH)
