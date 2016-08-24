# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os
import re

import common
from autotest_lib.client.common_lib import error
from autotest_lib.server import test


_DEFAULT_BLOCK_SIZE = 4096
_DEFAULT_NUM_BLOCKS = 20000
_DEFAULT_MIN_SPEED = 30 * 1024 * 1024


class brillo_StorageWriteSpeedTest(test.test):
    """Verify that writing to a Brillo device data storage is fast enough."""
    version = 1

    def run_once(self, host=None, block_size=_DEFAULT_BLOCK_SIZE,
                 num_blocks=_DEFAULT_NUM_BLOCKS, min_speed=_DEFAULT_MIN_SPEED):
        """Runs the test.

        @param host: A host object representing the DUT.
        @param block_size: The size of blocks to write in bytes.
        @param num_blocks: The number of blocks to write.
        @param min_speed: Minimum required write speed in bytes/sec.

        @raise TestError: Something went wrong while trying to execute the test.
        @raise TestFail: The test failed.
        """
        try:
            tmp_file = os.path.join(host.get_tmp_dir(), 'testfile')
            result = host.run_output(
                    'dd if=/dev/zero of=%s bs=%s count=%s 2>&1' %
                    (tmp_file, block_size, num_blocks))
            actual_speed = None
            for line in result.splitlines():
                match = re.match('.*\(([0-9]+) bytes/sec\)$', line)
                if match:
                    actual_speed = int(match.group(1))
                    break
            if actual_speed is None:
                raise error.TestError('Error finding storage write speed')
            logging.info('Actual write speed is %d bytes/sec', actual_speed)
            if actual_speed < int(min_speed):
                logging.error('Write speed (%s bytes/sec) is lower than '
                              'required (%s bytes/sec)',
                              actual_speed, min_speed)
                raise error.TestFail(
                        'Storage write speed is lower than required')
        except error.AutoservRunError:
            raise error.TestFail('Error writing to device data partition')
