# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.server.cros.faft.firmware_test import FirmwareTest


class firmware_CorruptKernelB(FirmwareTest):
    """
    Servo based kernel B corruption test.

    This test sets kernel B boot and then corrupts kernel B. The firmware
    verifies kernel B failed so falls back to kernel A boot. This test will
    fail if kernel verification mis-behaved.
    """
    version = 1

    def initialize(self, host, cmdline_args, dev_mode=False):
        super(firmware_CorruptKernelB, self).initialize(host, cmdline_args)
        self.backup_kernel()
        self.backup_cgpt_attributes()
        self.switcher.setup_mode('dev' if dev_mode else 'normal')
        self.setup_usbkey(usbkey=False)
        self.setup_kernel('a')

    def cleanup(self):
        self.restore_cgpt_attributes()
        self.restore_kernel()
        super(firmware_CorruptKernelB, self).cleanup()

    def run_once(self, dev_mode=False):
        logging.info("Prioritize kernel B.")
        self.check_state((self.checkers.root_part_checker, 'a'))
        self.reset_and_prioritize_kernel('b')
        self.switcher.mode_aware_reboot()

        logging.info("Expected kernel B boot and corrupt kernel B.")
        self.check_state((self.checkers.root_part_checker, 'b'))
        self.faft_client.kernel.corrupt_sig('b')
        self.switcher.mode_aware_reboot()

        logging.info("Expected kernel A boot and restore kernel B.")
        self.check_state((self.checkers.root_part_checker, 'a'))
        self.faft_client.kernel.restore_sig('b')
        self.switcher.mode_aware_reboot()

        logging.info("Expected kernel B boot and prioritize kerenl A.")
        self.check_state((self.checkers.root_part_checker, 'b'))
        self.reset_and_prioritize_kernel('a')
        self.switcher.mode_aware_reboot()

        logging.info("Expected kernel A boot.")
        self.check_state((self.checkers.root_part_checker, 'a'))
