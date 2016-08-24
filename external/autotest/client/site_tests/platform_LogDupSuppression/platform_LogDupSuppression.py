# Copyright 2014 The Chromium OS Authors. All rights reserved.  Use of
# this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import syslog

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import utils

class platform_LogDupSuppression(test.test):
    """Test that we suppress duplicate messages from one process to syslog"""
    DUP_DETECT_SIG = "spam: last message repeated"
    NON_SPAM_MSG = 'not spam'
    NUM_SPAM_MSGS = 10
    SPAM_LOG_PATH = '/var/log/spam.log'
    SPAM_MSG = 'SPAM SPAM SPAM'
    SYSLOG_BIN = 'rsyslogd'
    SYSLOG_OPTS = '-c4'  # allow version 4 commands
    SYSLOG_JOB = 'syslog'
    version = 1


    def run_once(self):
        syslog.openlog('spam')
        try:
            utils.run('stop %s' % self.SYSLOG_JOB,
                      ignore_status=True)  # might not have been running
            utils.run('truncate -s 0 %s' % self.SPAM_LOG_PATH)
            utils.run('chown syslog %s' % self.SPAM_LOG_PATH)
            utils.run('%s %s -f %s/rsyslog.test' %
                      (self.SYSLOG_BIN, self.SYSLOG_OPTS, self.bindir))

            for i in range(self.NUM_SPAM_MSGS):
                syslog.syslog(self.SPAM_MSG)
            syslog.syslog(self.NON_SPAM_MSG)

            cmd_result = utils.run(
                'grep "%s" %s' % (self.DUP_DETECT_SIG, self.SPAM_LOG_PATH),
                ignore_status=True)
            if cmd_result.exit_status:
                raise error.TestFail(
                    'duplicate suppression signature not found')

            spam_count = int(
                utils.run('grep -c "%s" %s' %
                          (self.SPAM_MSG, self.SPAM_LOG_PATH)).stdout)
            if spam_count != 1:
                raise error.TestFail(
                    'got %s spams, expected exactly one' % spam_count)
        finally:
            utils.run('pkill %s' % self.SYSLOG_BIN)
            utils.run('start %s' % self.SYSLOG_JOB)
