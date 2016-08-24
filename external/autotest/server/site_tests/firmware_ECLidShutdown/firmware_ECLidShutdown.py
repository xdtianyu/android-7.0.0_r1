# Copyright (c) 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import time

from autotest_lib.client.common_lib import error
from autotest_lib.server.cros import vboot_constants as vboot
from autotest_lib.server.cros.faft.firmware_test import FirmwareTest
from autotest_lib.server.cros.faft.utils import mode_switcher

class firmware_ECLidShutdown(FirmwareTest):
    """
    Testing GBB_FLAG_DISABLE_LID_SHUTDOWN flag
    """
    version = 1

    # Delay between closing and opening the lid
    LID_DELAY = 2
    # time to wait before checking if DUT booted into OS mode
    BOOTUP_TIME = 30
    # time to wait to let DUT transition into recovery mode
    RECOVERY_DELAY = 1
    # # times to check if DUT in expected power state
    # This accomodates if DUT needs to transition into certain states.
    PWR_RETRIES = 13

    def initialize(self, host, cmdline_args):
        super(firmware_ECLidShutdown, self).initialize(host, cmdline_args)
        self.setup_usbkey(usbkey=False)

    def cleanup(self):
        """If DUT not pingable, may be still stuck in recovery mode.
        Reboot it.  Also, reset GBB_FLAGS and make sure that lid set
        to open (in case of error).
        """
        # reset ec_uart_regexp to prevent timeouts in case there was
        # an error before we could reset it
        self._reset_ec_regexp()
        if self.servo.get('lid_open') == 'no':
            self.servo.set('lid_open', 'yes')
        self.clear_set_gbb_flags(vboot.GBB_FLAG_DISABLE_LID_SHUTDOWN,
                                 0)
        try:
            self.switcher.wait_for_client()
        except ConnectionError:
            logging.error("ERROR: client not in OS mode.  Rebooting ...")
            # reboot back to OS mode
            self.switcher.mode_aware_reboot(reboot_type='cold',
                                            sync_before_boot=False)
        super(firmware_ECLidShutdown, self).cleanup()

    def _reset_ec_regexp(self):
        """Reset ec_uart_regexp field

        Needs to be done for the ec_uart_regexp otherwise
        dut-control command will time out due to no match.
        """
        self.servo.set('ec_uart_regexp', 'None')

    def verify_lid_shutdown(self):
        """
        Make sure that firmware boots into OS with lid closed
        """
        self.clear_set_gbb_flags(vboot.GBB_FLAG_DISABLE_LID_SHUTDOWN,
                                 0)
        # reboot into recovery mode and wait a bit for it to actually get there
        self.faft_client.system.request_recovery_boot()
        self.switcher.mode_aware_reboot(wait_for_dut_up=False)
        time.sleep(self.RECOVERY_DELAY)

        # close/open lid
        self.servo.set('lid_open', 'no')
        time.sleep(self.LID_DELAY)
        if not self.wait_power_state("G3", self.PWR_RETRIES):
            logging.error("ERROR: EC does not shut down")
            return False
        self.servo.set('lid_open', 'yes')

        # ping DUT - should boot into OS now
        self._reset_ec_regexp()
        self.switcher.wait_for_client()

        return True

    def check_disable_lid_shutdown(self):
        """
        Set flag to disable shutdown of DUT when lid closed.  Then check
        if DUT shuts down during recovery mode screen.
        """
        # enable shutdown flag
        self.clear_set_gbb_flags(0,
                                 vboot.GBB_FLAG_DISABLE_LID_SHUTDOWN)
        # reboot into recovery mode and wait a bit for it to get there
        self.faft_client.system.request_recovery_boot()
        self.switcher.mode_aware_reboot(wait_for_dut_up=False)
        time.sleep(self.RECOVERY_DELAY)

        # close/open the lid
        self.servo.set('lid_open', 'no')
        time.sleep(self.LID_DELAY)
        if not self.wait_power_state("S0", self.PWR_RETRIES):
            logging.error("ERROR: EC shuts down")
            return False
        self.servo.set('lid_open', 'yes')

        # this should be more than enough time for system to boot up
        # if it was going to.
        time.sleep(self.BOOTUP_TIME)

        # should still be offline
        self.switcher.wait_for_client_offline()

        # reboot back to OS mode
        self.switcher.mode_aware_reboot(reboot_type='cold',
                                        sync_before_boot=False)

        # disable flag
        self._reset_ec_regexp()
        return True


    def run_once(self):
        if not self.check_ec_capability(['lid']):
            raise error.TestNAError("This device needs a lid to run this test")

        logging.info("Verify DUT with DISABLE_LID_SHUTDOWN disabled")
        self.check_state(self.verify_lid_shutdown)

        logging.info("Verify DUT with DISABLE_LID_SHUTDOWN enabled")
        self.check_state(self.check_disable_lid_shutdown)

