# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.client.common_lib import utils
from autotest_lib.server.cros.faft.firmware_test import FirmwareTest


class firmware_ConsecutiveBootPowerButton(FirmwareTest):
    """
    Servo based consecutive boot test via power button for both on and off.

    This test is intended to be run with many iterations to ensure that the DUT
    does boot into Chrome OS and then does power off later.

    The iteration should be specified by the parameter -a "faft_iterations=10".
    """
    version = 1


    def initialize(self, host, cmdline_args, dev_mode=False):
        # Parse arguments from command line
        dict_args = utils.args_to_dict(cmdline_args)
        self.faft_iterations = int(dict_args.get('faft_iterations', 1))
        super(firmware_ConsecutiveBootPowerButton,
              self).initialize(host, cmdline_args)
        self.switcher.setup_mode('dev' if dev_mode else 'normal')
        self.setup_usbkey(usbkey=False)


    def run_once(self, dev_mode=False):
        for i in xrange(self.faft_iterations):
            logging.info('======== Running FAFT ITERATION %d/%s ========',
                         i+1, self.faft_iterations)
            logging.info("Expected boot fine, full power off DUT and on.")
            self.check_state((self.checkers.crossystem_checker, {
                        'mainfw_type': 'developer' if dev_mode else 'normal',
                        }))
            self.full_power_off_and_on()
            self.switcher.wait_for_client()

            logging.info("Expected boot fine.")
            self.check_state((self.checkers.crossystem_checker, {
                        'mainfw_type': 'developer' if dev_mode else 'normal',
                        }))
