# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import sys
from autotest_lib.client.bin import test, utils


_PLATFORM_MAPPINGS = {'daisy': 'snow',
                      'daisy_spring': 'spring',
                      'x86-alex': 'alex',
                      'x86-mario': 'mario',
                      'x86-zgb': 'zgb'}


class platform_GesturesRegressionTest(test.test):
    """ Wrapper of regression test of gestures library.

    This test takes advantage of autotest framework to execute the touchtests,
    i.e. regression test of gestures library, and store results of the test
    per build(as one of BVTs) for us to keep track of patches of gestures
    library and regression tests, and their score changes accordingly.
    """
    version = 1

    def setup(self):
        self.job.setup_dep(['touchpad-tests'])

    def run_once(self):
        """ Run the regression test and collect the results.
        """
        board = utils.get_current_board()
        platform = _PLATFORM_MAPPINGS.get(board, board)

        # find paths for touchpad tests
        root = os.path.join(self.autodir, 'deps', 'touchpad-tests')
        framework_dir = os.path.join(root, 'framework')
        tests_dir = os.path.join(root, 'tests')

        # create test runner
        sys.path.append(framework_dir)
        sys.path.append(root)
        from test_runner import ParallelTestRunner
        runner = ParallelTestRunner(tests_dir)

        # run all tests for this platform and extract results
        results = runner.RunAll('%s*/*' % platform, verbose=True)
        # TODO(dennisjeffrey): Remove all uses of self.test_results below,
        # including the call to self.write_perf_keyval(), once we're ready to
        # switch over completely from perf keyvals to output_perf_value().
        self.test_results = {}
        for key, value in results.items():
            score = value['score']
            not_integer = isinstance(score, bool) or not isinstance(score, int)
            if not_integer and not isinstance(score, float):
                score = 0.0
            self.test_results[key.replace('/', '-')] = score
            self.output_perf_value(key.replace('/', '-'), score, 'points')

        # write converted test results out
        if self.test_results:
            self.write_perf_keyval(self.test_results)
