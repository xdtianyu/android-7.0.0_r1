#!/usr/bin/python
#
# Copyright (c) 2010 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

__author__ = 'kdlucas@chromium.org (Kelly Lucas)'

import logging, os

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error


class platform_TempFS(test.test):
    """
    Test temp file systems.
    """
    version = 1

    def run_once(self):
        errors = 0
        # The minimum available space we expect on temp filesystems.
        # TempFS allows 1/2 of Total Memory for each temp fs. Our threshold
        # allows for 50% usage of space allocated before this test is run.

        threshold = utils.memtotal()/4
        tempdirs = ['/dev', '/tmp', '/dev/shm', '/var/tmp', '/var/run',
                    '/var/lock']

        for dir in tempdirs:
            if os.path.isdir(dir):
                # utils.freespace is in bytes, so convert to kb.
                avail = utils.freespace(dir)/1024
                if avail < threshold:
                    logging.error('Not enough available space on %s', dir)
                    logging.error('%d bytes is minimum, found %d bytes',
                                  (threshold, avail))
                    errors += 1
            else:
                logging.error('%s does not exist!' % dir)
                errors += 1

        if errors:
            raise error.TestFail('There were %d temp directory errors' % errors)
