# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error

SYSTEM_SBIN = '/usr/sbin'
LIVE_TEST_LIST = ['chapsd_test', 'tpm_utility_test']

class platform_Pkcs11LiveTest(test.test):
    """
    This test runs the Chaps Live tests on a device with a TPM.

    Currently we have two test suits that run.
    1) chapsd_test
    2) tpm_utility_test
    """

    version = 1

    def run_once(self):
        for live_test in LIVE_TEST_LIST:
            test_path = os.path.join(SYSTEM_SBIN, live_test)
            exit_status = utils.system(test_path, ignore_status=True)
            if (exit_status != 0):
                raise error.TestFail(live_test + " has failures")
