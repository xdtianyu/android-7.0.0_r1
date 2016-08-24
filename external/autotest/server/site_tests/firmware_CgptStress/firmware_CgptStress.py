# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.client.common_lib import utils
from autotest_lib.server.cros.faft.firmware_test import FirmwareTest


class firmware_CgptStress(FirmwareTest):
    """
    Servo based, iterative cgpt test. One iteration of test modifies CGPT to
    switch to boot kernel B and then switch back to kernel A again.
    """
    version = 1

    def initialize(self, host, cmdline_args, dev_mode):
        # Parse arguments from command line
        dict_args = utils.args_to_dict(cmdline_args)
        self.faft_iterations = int(dict_args.get('faft_iterations', 1))
        super(firmware_CgptStress, self).initialize(host, cmdline_args)
        self.backup_cgpt_attributes()
        self.switcher.setup_mode('dev' if dev_mode else 'normal')
        self.setup_usbkey(usbkey=False)
        self.setup_kernel('a')

    def cleanup(self):
        self.restore_cgpt_attributes()
        super(firmware_CgptStress, self).cleanup()

    def run_once(self):
        for i in xrange(self.faft_iterations):
            logging.info('======== Running FAFT ITERATION %d/%s ========',
                         i + 1, self.faft_iterations)
            logging.info("Expected kernel A boot and prioritize kernel B.")
            self.check_state((self.checkers.root_part_checker, 'a'))
            self.reset_and_prioritize_kernel('b')
            self.switcher.mode_aware_reboot()

            logging.info("Expected kernel B boot and prioritize kernel A.")
            self.check_state((self.checkers.root_part_checker, 'b'))
            self.reset_and_prioritize_kernel('a')
            self.switcher.mode_aware_reboot()

            logging.info("Expected kernel A boot, done.")
            self.check_state((self.checkers.root_part_checker, 'a'))
