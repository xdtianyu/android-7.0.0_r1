# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os

from autotest_lib.client.bin import test
from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error


_EXPECTED_FILE = 'expected'


class platform_KernelVersionByBoard(test.test):
    """ Compare kernel version of a build to expected value. """

    version = 1

    def _expected_kernel(self, board):
        """ Return expected kernel version number from file.

        @return: string of expected kernel version (e.g. '3.4')
        """
        with open(os.path.join(self.bindir, _EXPECTED_FILE)) as file_handle:
            for line in file_handle:
                file_board, expected = line.split()
                if board == file_board:
                    logging.info('Expected Kernel Version for %s: %s', board,
                                 expected)
                    return expected.strip()

        actual = self._actual_kernel(board)
        raise error.TestError('Could not find expected kernel version for '
                              '%s.  Should it be added?  Actual is %s.'
                              % (board, actual))

    def _actual_kernel(self, board):
        """ Return actual kernel version number from device.

        @returns: string of actual kernel version (e.g. '3.8.11')
        """
        return utils.system_output('uname -r')

    def run_once(self):
        """ Compare expected and actual kernel versions. """
        board = utils.get_current_board()
        actual = self._actual_kernel(board)
        expected = self._expected_kernel(board)
        if not actual.startswith(expected):
            raise error.TestFail('%s: Expected kernel version %s; Found %s'
                                 % (board, expected, actual))

