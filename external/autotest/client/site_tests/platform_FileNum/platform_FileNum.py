#!/usr/bin/python
#
# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
This testcase exercises the file system by ensuring we can create a sufficient
number of files into one directory. In this case we will create 150,000 files on
the stateful partition and 2,000 files on the /tmp partition.
"""

__author__ = ['kdlucas@chromium.org (Kelly Lucas)',
              'dalecurtis@chromium.org (Dale Curtis)']

import os
import shutil

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error


class platform_FileNum(test.test):
    """Test file number limitations per directory."""
    version = 1

    _TEST_PLAN = [
        {'dir': '/mnt/stateful_partition', 'count': 150000},
        {'dir': '/tmp', 'count': 2000}]

    _TEST_TEXT = 'ChromeOS rocks with fast response and low maintenance costs!'

    def create_files(self, target_dir, count):
        """Create the number of files specified by count in target_dir.

        Args:
            target_dir: Directory to create files in.
            count: Number of files to create.
        Returns:
            Number of files created.
        """
        create_dir = os.path.join(target_dir, 'createdir')
        try:
            if os.path.exists(create_dir):
                shutil.rmtree(create_dir)

            os.makedirs(create_dir)

            for i in xrange(count):
                f = open(os.path.join(create_dir, '%d.txt' % i), 'w')
                f.write(self._TEST_TEXT)
                f.close()

            total_created = len(os.listdir(create_dir))
        finally:
            shutil.rmtree(create_dir)

        return total_created

    def run_once(self):
        for item in self._TEST_PLAN:
            actual_count = self.create_files(item['dir'], item['count'])
            if actual_count != item['count']:
                raise error.TestFail(
                    'File creation count in %s is incorrect! Found %d files '
                    'when there should have been %d!'
                    % (item['dir'], actual_count, item['count']))
