# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import time

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import utils
from autotest_lib.server.cros import vboot_constants as vboot
from autotest_lib.server.cros.faft.firmware_test import FirmwareTest
from autotest_lib.server.cros.faft.firmware_test import ConnectionError


class firmware_ConsecutiveBoot(FirmwareTest):
    """
    Servo based consecutive boot test via power button to turn on DUT and
    /sbin/shutdown command to turn off DUT.

    This test is intended to be run with many iterations to ensure that the DUT
    does boot into Chrome OS and then does power off later.

    The iteration should be specified by the parameter -a "faft_iterations=10".
    """
    version = 1
    # Number of power button press to boot before declare test fail.
    POWER_ON_RETRY = 3


    def initialize(self, host, cmdline_args, dev_mode=False):
        # Parse arguments from command line
        dict_args = utils.args_to_dict(cmdline_args)
        self.faft_iterations = int(dict_args.get('faft_iterations', 1))
        self.faft_waitup_time = int(dict_args.get('faft_waitup_time', 0))
        self.faft_localrun = int(dict_args.get('faft_localrun', 0))
        super(firmware_ConsecutiveBoot, self).initialize(host, cmdline_args)
        self.switcher.setup_mode('dev' if dev_mode else 'normal')
        if dev_mode:
          self.clear_set_gbb_flags(0, vboot.GBB_FLAG_DEV_SCREEN_SHORT_DELAY)
        self.setup_usbkey(usbkey=False)

    def wait_for_client_aux(self):
        """Use test specific timeout to wait for system to come up,
           otherwise use default (180s).
        """
        logging.info('wait_for_client %d start.', self.faft_waitup_time)
        if self.faft_waitup_time:
            self.switcher.wait_for_client(self.faft_waitup_time)
        else:
            self.switcher.wait_for_client()

    def shutdown_power_on(self):
        """
        Use /sbin/shutdown to turn off device follow by power button press to
        turn on device.  Do not want to call full_power_off_and_on since we
        are testing firmware and mainly want to focus on power on sequence.
        """
        boot_id = self.get_bootid()
        # Call shutdown instead of long press the power key since we are
        # testing the firmware and not the OS.
        logging.info("Sending /sbin/shutdown -P now")
        self.faft_client.system.run_shell_command('/sbin/shutdown -P now')
        logging.info('Wait for client to go offline')
        self.switcher.wait_for_client_offline(timeout=100, orig_boot_id=boot_id)
        # Retry in case power_short_press was not registered.
        for i in xrange(self.POWER_ON_RETRY):
            logging.info("sleep %d, tap power key to boot.",
                         self.faft_config.powerup_ready)
            time.sleep(self.faft_config.powerup_ready)
            self.servo.power_key(self.faft_config.hold_pwr_button_poweron)
            try:
                self.wait_for_client_aux()
            except ConnectionError:
                logging.error('wait_for_client exception %d.', i)
            else:
                logging.info('wait_for_client online done %d.', i)
                return
        raise ConnectionError()

    def run_once(self, host, dev_mode=False):
        for i in xrange(self.faft_iterations):
            logging.info('======== Running FAFT ITERATION %d/%s ========',
                         i+1, self.faft_iterations)
            logging.info("Expected boot fine, full power off DUT and on.")
            self.check_state((self.checkers.crossystem_checker, {
                        'mainfw_type': 'developer' if dev_mode else 'normal',
                        }))
            self.shutdown_power_on()

            logging.info("Expected boot fine.")
            self.check_state((self.checkers.crossystem_checker, {
                        'mainfw_type': 'developer' if dev_mode else 'normal',
                        }))
