# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.server.cros.faft.firmware_test import FirmwareTest


class firmware_DevMode(FirmwareTest):
    """
    Servo based developer firmware boot test.
    """
    version = 1

    def initialize(self, host, cmdline_args, ec_wp=None):
        super(firmware_DevMode, self).initialize(host, cmdline_args,
                                                 ec_wp=ec_wp)
        self.switcher.setup_mode('normal')
        self.setup_usbkey(usbkey=False)

    def run_once(self):
        logging.info("Enable dev mode.")
        self.check_state((self.checkers.crossystem_checker, {
                              'devsw_boot': '0',
                              'mainfw_type': 'normal',
                              }))
        self.switcher.reboot_to_mode(to_mode='dev')

        logging.info("Expected developer mode boot and enable normal mode.")
        self.check_state((self.checkers.crossystem_checker, {
                              'devsw_boot': '1',
                              'mainfw_type': 'developer',
                              }))
        self.switcher.reboot_to_mode(to_mode='normal')

        logging.info("Expected normal mode boot, done.")
        self.check_state((self.checkers.crossystem_checker, {
                              'devsw_boot': '0',
                              'mainfw_type': 'normal',
                              }))
