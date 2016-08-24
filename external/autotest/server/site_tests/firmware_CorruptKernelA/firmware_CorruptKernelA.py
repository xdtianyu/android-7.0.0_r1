# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.server.cros.faft.firmware_test import FirmwareTest


class firmware_CorruptKernelA(FirmwareTest):
    """
    Servo based kernel A corruption test.

    This test corrupts kernel A and checks for kernel B on the next boot.
    It will fail if kernel verification mis-behaved.
    """
    version = 1

    def initialize(self, host, cmdline_args, dev_mode=False):
        super(firmware_CorruptKernelA, self).initialize(host, cmdline_args)
        self.backup_kernel()
        self.backup_cgpt_attributes()
        self.switcher.setup_mode('dev' if dev_mode else 'normal')
        self.setup_usbkey(usbkey=False)
        self.setup_kernel('a')

    def cleanup(self):
        self.restore_cgpt_attributes()
        self.restore_kernel()
        super(firmware_CorruptKernelA, self).cleanup()

    def run_once(self, dev_mode=False):
        logging.info("Corrupt kernel A.")
        self.check_state((self.checkers.root_part_checker, 'a'))
        self.faft_client.kernel.corrupt_sig('a')
        self.switcher.mode_aware_reboot()

        logging.info("Expected kernel B boot and restore kernel A.")
        self.check_state((self.checkers.root_part_checker, 'b'))
        self.faft_client.kernel.restore_sig('a')
        self.switcher.mode_aware_reboot()

        logging.info("Expected kernel A boot.")
        self.check_state((self.checkers.root_part_checker, 'a'))
