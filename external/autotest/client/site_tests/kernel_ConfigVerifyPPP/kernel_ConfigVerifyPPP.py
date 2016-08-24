# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import kernel_config

class kernel_ConfigVerifyPPP(test.test):
    """Checks that PPP modules are present.
    """
    version = 1
    IS_MODULE = [
        # Sanity checks; should be present in builds as modules.
        'PPPOE',
        'PPP_MPPE',
        'PPP_BSDCOMP',
        'PPP_DEFLATE',
        'PPP_SYNC_TTY',
    ]

    def run_once(self):
        # Load the list of kernel config variables.
        config = kernel_config.KernelConfig()
        config.initialize()

        # Run the static checks.
        map(config.has_module, self.IS_MODULE)

        # Raise a failure if anything unexpected was seen.
        if len(config.failures()):
            raise error.TestFail((", ".join(config.failures())))
