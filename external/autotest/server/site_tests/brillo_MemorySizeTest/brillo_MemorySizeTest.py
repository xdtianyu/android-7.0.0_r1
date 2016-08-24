# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import common
from autotest_lib.client.common_lib import error
from autotest_lib.server import test


_DEFAULT_MIN_TOTAL = 32 * 1024
_DEFAULT_MIN_FREE = 4 * 1024


class brillo_MemorySizeTest(test.test):
    """Verify that a Brillo device has enough memory."""
    version = 1

    def run_once(self, host=None, min_total=_DEFAULT_MIN_TOTAL,
                 min_free=_DEFAULT_MIN_FREE):
        """Check that there's enough total and free memory on the device.

        @param host: a host object representing the DUT.
        @param min_total: Minimum total memory in 1K blocks.
        @param min_free: Minimum free memory in 1K blocks.

        @raise TestError: Something went wrong while trying to execute the test.
        @raise TestFail: The test failed.
        """
        if int(min_total) < int(min_free):
            raise error.TestError(
                    'Requiring more free memory (%s) than total memory (%s)' %
                    (min_free, min_total))

        try:
            meminfo_output = host.run_output('cat /proc/meminfo').splitlines()
        except error.AutoservRunError:
            raise error.TestFail('Failed to cat /proc/meminfo')

        meminfo_dict = dict([[tok.strip() for tok in line.split(':', 1)]
                             for line in meminfo_output])

        for mem_type, min_mem in (('MemTotal', min_total), ('MemFree', min_free)):
            actual_mem = meminfo_dict.get(mem_type)
            if not (actual_mem and actual_mem.endswith(' kB')):
                raise error.TestFail(
                        'Failed to read %s from /proc/meminfo' % mem_type)
            actual_mem = actual_mem.split()[0]
            if int(actual_mem) < int(min_mem):
                raise error.TestFail(
                        '%s (%s kB) is less than required (%s kB)' %
                        (mem_type, actual_mem, min_mem))
