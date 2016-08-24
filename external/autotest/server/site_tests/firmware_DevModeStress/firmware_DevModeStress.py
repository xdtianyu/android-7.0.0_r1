# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

#from threading import Timer
import logging
import time

from autotest_lib.client.common_lib import utils
from autotest_lib.server.cros.faft.firmware_test import FirmwareTest


class firmware_DevModeStress(FirmwareTest):
    """
    Servo based, iterative developer firmware boot test. One iteration
    of this test performs 2 reboots and 3 checks.
    """
    version = 1

    def initialize(self, host, cmdline_args):
        # Parse arguments from command line
        dict_args = utils.args_to_dict(cmdline_args)
        self.faft_iterations = int(dict_args.get('faft_iterations', 1))
        super(firmware_DevModeStress, self).initialize(host, cmdline_args)
        self.switcher.setup_mode('dev')
        self.setup_usbkey(usbkey=False)

    def run_once(self):
        for i in xrange(self.faft_iterations):
            logging.info('======== Running FAFT ITERATION %d/%s ========',
                         i + 1, self.faft_iterations)
            logging.info("Verify dev mode.")
            self.check_state((self.checkers.crossystem_checker, {
                                'devsw_boot': '1',
                                'mainfw_type': 'developer',
                                }))
            self.switcher.mode_aware_reboot()

            logging.info("Verify dev mode after soft reboot.")
            self.check_state((self.checkers.crossystem_checker, {
                                'devsw_boot': '1',
                                'mainfw_type': 'developer',
                                }))
            self.switcher.mode_aware_reboot(
                    'custom',
                    lambda:self.suspend_as_reboot(self.wake_by_power_button))

        logging.info("Complete, final check for dev mode.")
        self.check_state((self.checkers.crossystem_checker, {
                            'devsw_boot': '1',
                            'mainfw_type': 'developer',
                            }))
