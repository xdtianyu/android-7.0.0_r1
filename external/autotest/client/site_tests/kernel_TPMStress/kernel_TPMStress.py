# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import service_stopper


class kernel_TPMStress(test.test):
    "TPM communication stress test"
    version = 1


    def initialize(self):
        self._services = service_stopper.ServiceStopper(['cryptohomed',
                                                         'chapsd', 'tcsd'])
        self._services.stop_services()


    def run_once(self):
        # On a Mario, running the test with 1000 iterations takes 89 seconds,
        # and with 2000 iterations 163 seconds, i.e. the incremental time for 1
        # iteration is 74ms.
        n_iterations = 3000

        try:
            for iteration in range(n_iterations):
                utils.system("tpmc getpf")
        except Exception:
            raise error.TestError(("TPM communication error at " +
                                   "iteration %d of %d") %
                                  (iteration, n_iterations))


    def cleanup(self):
        self._services.restore_services()
