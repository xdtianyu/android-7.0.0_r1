#!/usr/bin/env python
# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# This module brings together the magic to setup logging correctly for
# pseudomodem.

import logging
import logging.handlers
import os
import sys

SYSLOG_DEVICE = '/dev/log'

class ModemManagerFormatter(logging.Formatter):
    """
    Format log strings such that rsyslogd handles them correctly.

    By adding a prefix 'ModemManager[xxx]' where |xxx| contains the pid, we can
    ensure that rsyslogd treats the messages the same way it treats messages
    from ModemManager.

    """
    def __init__(self, *args, **kwargs):
        super(ModemManagerFormatter, self).__init__(*args, **kwargs)
        self._pid = os.getpid()


    def format(self, record):
        """
        The main function that converts log records to strings.

        @param record: The log record.
        @returns: The formatted log string.

        """
        result = super(ModemManagerFormatter, self).format(record)
        return 'pseudomodem[%d]: %s' % (self._pid, result)


def SetupLogging():
    """
    The main function that sets up logging as expected. It does the following:

    (1) Clear out existing logging setup that leaks in during autotest import.
    (2) Setup logging handler to log to stdout
    (3) Setup logging handler to log to syslog.

    """
    root = logging.getLogger()
    for handler in root.handlers:
        root.removeHandler(handler)

    stdout_handler = logging.StreamHandler(sys.stdout)
    stdout_formatter = logging.Formatter(
        fmt='%(asctime)s %(levelname)-5.5s| %(message)s',
        datefmt='%H:%M:%S')
    stdout_handler.setFormatter(stdout_formatter)
    root.addHandler(stdout_handler)

    syslog_handler = logging.handlers.SysLogHandler(
            SYSLOG_DEVICE,
            facility=logging.handlers.SysLogHandler.LOG_DAEMON)
    syslog_formatter = ModemManagerFormatter(
        fmt='%(levelname)-5.5s|%(module)10.10s:%(lineno)4.4d| %(message)s')
    syslog_handler.setFormatter(syslog_formatter)
    root.addHandler(syslog_handler)
