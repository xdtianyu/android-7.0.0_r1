# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, os

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error

class hardware_Memtester(test.test):
    """
    This test uses memtester to find memory subsystem faults. Amount of memory
    to test is all of the free memory plus buffer and cache region with 30 MB
    reserved for OS use.
    """

    version = 1

    def run_once(self, size=0, loop=10):
        """
        Executes the test and logs the output.

        @param size: size to test in KB. 0 means all usable
        @param loop: number of iteration to test memory
        """
        if size == 0:
            size = utils.usable_memtotal()
        elif size > utils.memtotal():
            raise error.TestFail('Specified size is more than total memory.')

        if size <= 0:
            raise error.TestFail('Size must be more than zero.')


        logging.info('Memory test size: %dK', size)

        cmd = 'memtester %dK %d' % (size, loop)
        logging.info('cmd: %s', cmd)

        with open(os.path.join(self.resultsdir, 'memtester_stdout'), 'w') as f:
            utils.run(cmd, stdout_tee=f)
