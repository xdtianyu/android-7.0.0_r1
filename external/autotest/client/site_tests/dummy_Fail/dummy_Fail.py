# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error

# A global variable to track how many times the test has been run.
global retry_count
retry_count = 0

class dummy_Fail(test.test):
    """The test fails by raising given exception, or succeeds at given retry."""
    version = 1

    def run_once(self, to_throw=None, retry_success_count=0):
        """Run test with argument to_throw, retry_count and retry_success_count.

        @param to_throw: Exception to throw in the test.
        @param retry_success_count: The number of times to fail before test is
                    completed successfully. 0 means the test will never complete
                    successfully with reties.

        """
        global retry_count
        retry_count += 1
        if retry_count == retry_success_count:
            return
        if to_throw:
            if to_throw == 'TestFail': logging.error('It is an error!')
            raise getattr(error, to_throw)('always fail')
        else:  # Generate a crash to test that behavior.
            self.write_perf_keyval({'perf_key': 102.7})
            self.job.record('INFO', self.tagged_testname,
                            'Received crash notification for sleep[273] sig 6')
