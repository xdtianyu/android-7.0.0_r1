#!/usr/bin/python
#
# Copyright (c) 2010 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

__author__ = 'kdlucas@chromium.org (Kelly Lucas)'

import logging, os

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error


class platform_CheckErrorsInLog(test.test):
    """
    Check system logs for errors.
    """
    version = 1

    def search_log(self, logfile):
        """
        Try to ping the remote host and report the status.
        Args:
            logfile: string, pathname of logfile to search.
        Returns:
            integer: number of errors found.
        """
        errors = 0
        kerrors = ['fatal', 'oops', 'panic', 'segfault']
        f = open(logfile, 'r')
        log = f.readlines()
        for line in log:
            for key in kerrors:
                if key in line:
                    errors += 1
                    logging.error('%s found in %s' ,line, logfile)
        f.close()

        return errors


    def run_once(self):
        errors = 0
        logs = ['kern.log', 'syslog', 'dmesg']

        for log in logs:
            logfile = os.path.join('/var/log', log)
            if os.path.isfile(logfile):
                errors += self.search_log(logfile)
            else:
                logging.warning('%s does not exist' % logfile)

        if errors:
            raise error.TestFail('%d failures found in logs' % errors)
