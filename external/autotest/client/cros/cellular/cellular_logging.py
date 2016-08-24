#!/usr/bin/python
# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import sys

LOG_FORMAT = ' %(name)s - %(filename)s - %(lineno)d- %(message)s'


def SetupCellularLogging(logger_name, format_string=LOG_FORMAT):
    """
    logger_name: The name of the logger. This name can be used across files
      to access the same logger.
    format_string: The format of the log message this logger prints.
    returns: a log object that may be used :
      log.debug('Print this at the debug level ')
    """
    log = logging.getLogger(logger_name)
    log.setLevel(logging.DEBUG)
    ch = logging.StreamHandler(sys.stdout)
    ch.setLevel(logging.DEBUG)
    formatter = logging.Formatter(format_string)
    ch.setFormatter(formatter)
    log.handlers = [ch]
    log.propagate = False
    return log
