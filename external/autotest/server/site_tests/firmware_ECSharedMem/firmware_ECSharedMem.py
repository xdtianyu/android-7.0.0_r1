# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import time

from autotest_lib.client.common_lib import error
from autotest_lib.server.cros.faft.firmware_test import FirmwareTest


class firmware_ECSharedMem(FirmwareTest):
    """
    Servo based EC shared memory test.
    """
    version = 1

    def initialize(self, host, cmdline_args):
        super(firmware_ECSharedMem, self).initialize(host, cmdline_args)
        # Only run in normal mode
        self.switcher.setup_mode('normal')
        self.ec.send_command("chan 0")

    def cleanup(self):
        self.ec.send_command("chan 0xffffffff")
        super(firmware_ECSharedMem, self).cleanup()

    def shared_mem_checker(self):
        match = self.ec.send_command_get_output("shmem",
                                                ["Size:\s+([0-9-]+)\r"])[0]
        shmem_size = int(match[1])
        logging.info("EC shared memory size if %d bytes", shmem_size)
        if shmem_size <= 0:
            return False
        elif shmem_size <= 256:
            logging.warning("EC shared memory is less than 256 bytes")
        return True

    def jump_checker(self):
        self.ec.send_command("sysjump RW")
        time.sleep(self.faft_config.ec_boot_to_console)
        return self.shared_mem_checker()

    def run_once(self):
        if not self.check_ec_capability():
            raise error.TestNAError("Nothing needs to be tested on this device")

        logging.info("Check shared memory in normal operation and crash EC.")
        self.check_state(self.shared_mem_checker)
        self.switcher.mode_aware_reboot(
                'custom', lambda:self.ec.send_command('crash unaligned'))

        logging.info("Check shared memory after crash and system jump.")
        self.check_state([self.shared_mem_checker, self.jump_checker])
        self.switcher.mode_aware_reboot('custom', self.sync_and_ec_reboot)
