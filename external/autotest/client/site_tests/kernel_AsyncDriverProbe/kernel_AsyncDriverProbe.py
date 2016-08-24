# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import subprocess
from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error

class kernel_AsyncDriverProbe(test.test):
    """
    Handle checking asynchronous device probing.
    """
    version = 1

    def module_loaded(self, module):
        """
        Detect if the given module is already loaded in the kernel.

        @param module: name of module to check
        """
        module = module.replace('-', '_')
        match = "%s " % (module)
        for line in open("/proc/modules"):
            if line.startswith(match):
                return True
        return False

    def rmmod(self, module):
        """
        Unload a module if it is already loaded in the kernel.

        @param module: name of module to unload
        """
        if self.module_loaded(module):
            subprocess.call(["rmmod", module])

    def run_once(self):
        """
        Try loading the test module. It will time registration for
        synchronous and asynchronous cases and will fail to load if
        timing is off.
        """

        module = "test_async_driver_probe"

        # Start from a clean slate.
        self.rmmod(module)

        exit_code = subprocess.call(["modprobe", "-n", "-q", module])
        if exit_code:
            raise error.TestNAError(
                "%s.ko module does not seem to be available "
                "(modprobe rc=%s); skipping async probe test" %
                (module, exit_code))

        # Try loading the module. If it loads successfully test passes.
        subprocess.call(["modprobe", module])
        loaded = self.module_loaded(module)

        # Clean up after ourselves
        self.rmmod(module)

        if not loaded:
            raise error.TestFail("Test module failed to load")
