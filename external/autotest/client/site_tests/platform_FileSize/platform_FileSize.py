#!/usr/bin/python
#
# Copyright (c) 2010 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
This testcase exercises the filesystem by creating files of a specified size
and verifying the files are actually created to specification. This test will
ensure we can create a 1gb size file on the stateful partition, and a 100mb
size file on the /tmp partition.
"""

__author__ = 'kdlucas@chromium.org (Kelly Lucas)'

import os
import sys

from autotest_lib.client.bin import utils, test
from autotest_lib.client.common_lib import error


class platform_FileSize(test.test):
    """Test creating large files on various file systems."""
    version = 1

    def create_file(self, size, fname):
        """
        Create a file with the specified size.

        Args:
            size: int, size in megabytes
            fname: string, filename to create
        Returns:
            int, size of file created.
        """
        TEXT = 'ChromeOS knows how to make your netbook run fast!\n'
        count = size * 20000
        fh = file(fname, 'w')
        for i in range(count):
            fh.write(TEXT)
        fh.close()

        if os.path.exists(fname):
            fsize = os.path.getsize(fname)
            os.remove(fname)
            return fsize
        raise error.TestFail('Error, %s not found' % fname)

    def run_once(self):
        reqsize = [1024, 100]
        reqname = ['/mnt/stateful_partition/tempfile', '/tmp/tempfile']
        m = 1000000

        for i in range(2):
            filesize = self.create_file(reqsize[i], reqname[i])
            if not (filesize == (reqsize[i] * m)):
                raise error.TestFail('%s file test failed.' % reqname[i])
