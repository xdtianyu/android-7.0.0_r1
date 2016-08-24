# Copyright (c) 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import kernel_config


class kernel_FirmwareRequest(test.test):
    """
    Test asynchronous firmware loading
    """
    version = 1

    def set_module_locking(self, enabled):
        """
        Enable/disable LSM request_firmware location locking
        Inspired by security_ModuleLocking
        """
        sysctl = "/proc/sys/kernel/chromiumos/module_locking"
        value = '1\n' if enabled else '0\n'

        if os.path.exists(sysctl):
            open(sysctl, "w").write(value)
        else:
            raise error.TestNAError("module locking sysctl not available; may not be able to load test FW")


    def test_is_valid(self):
        """
        Check if this test is worth running, based on whether the kernel
        .config has the right features
        """
        config = kernel_config.KernelConfig()
        config.initialize()
        config.is_enabled('TEST_FIRMWARE')
        return len(config.failures()) == 0


    def do_fw_test(self):
        """
        Run one iteration of the test
        Return non-zero if failed
        """
        os.chdir(self.srcdir)
        ret = utils.system("./fw_filesystem.sh", ignore_status=True)
        if ret:
            raise error.TestFail("FW request test failed: %d" % (ret))


    def run_once(self):
        """
        This test will run the firmware request kernel self test (from
        upstream). This tests that the request_firmware() and
        request_firmware_nowait() kernel APIs are somewhat sane. It tries to
        load the empty filename ("") as well as a small toy firmware, and
        checks that it matches. It also makes sure a non-existent firmware
        cannot be found.

        We rerun the same test several times to increase the probability of
        catching errors.

        Needs to disable module locking so we can load test firmwares from
        non-standard locations (e.g., /tmp)
        """

        num_loops = 50
        module_name = "test_firmware"

        if not self.test_is_valid():
            raise error.TestNAError("FW test module is not available for this test")

        utils.load_module(module_name)
        if not utils.module_is_loaded(module_name):
            raise error.TestNAError("FW test module is not available for this test")

        try:
            self.set_module_locking(False)

            logging.info("iterations: %d", num_loops)

            for i in range(0, num_loops):
                self.do_fw_test()

        finally:
            self.set_module_locking(True)
            utils.unload_module(module_name)
