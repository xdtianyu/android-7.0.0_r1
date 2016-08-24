# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import time

from autotest_lib.client.common_lib import error
from autotest_lib.server.cros.faft.firmware_test import FirmwareTest


class firmware_DevScreenTimeout(FirmwareTest):
    """
    Servo based developer firmware screen timeout test.

    When booting in developer mode, the firmware shows a screen to warn user
    the disk image is not secured. If a user press Ctrl-D or a timeout reaches,
    it will boot to developer mode. This test is to verify the timeout period.

    This test tries to boot the system in developer mode twice.
    The first one will repeatly press Ctrl-D on booting in order to reduce
    the time on developer warning screen. The second one will do nothing and
    wait the developer screen timeout. The time difference of these two boots
    is close to the developer screen timeout.
    """
    version = 1

    # We accept 5s timeout margin as we need 5s to ensure client is offline.
    # If the margin is too small and firmware initialization is too fast,
    # the test will fail incorrectly.
    TIMEOUT_MARGIN = 5

    fw_time_record = {}

    def ctrl_d_repeatedly(self):
        """Press Ctrl-D repeatedly. We want to be aggressive and obtain a
        low firmware boot time when developer mode is enabled, so spam the
        AP console with ctrl-D every half second until the firmware_screen
        delay has been reached.
        """
        for _ in range(self.faft_config.firmware_screen * 2):
            self.servo.ctrl_d()
            time.sleep(0.5)

    def record_fw_boot_time(self, tag):
        """Record the current firmware boot time with the tag.

        Args:
          tag: A tag about this boot.

        Raises:
          error.TestError: If the firmware-boot-time file does not exist.
        """
        [fw_time] = self.faft_client.system.run_shell_command_get_output(
                'cat /tmp/firmware-boot-time')
        logging.info('Got firmware boot time: %s', fw_time)
        if fw_time:
            self.fw_time_record[tag] = float(fw_time)
        else:
            raise error.TestError('Failed to get the firmware boot time.')

    def check_timeout_period(self):
        """Check the firmware screen timeout period matches our spec.

        Raises:
          error.TestFail: If the timeout period does not match our spec.
        """
        # Record the boot time of firmware screen timeout.
        self.record_fw_boot_time('timeout_boot')
        got_timeout = (self.fw_time_record['timeout_boot'] -
                       self.fw_time_record['ctrl_d_boot'])
        logging.info('Estimated developer firmware timeout: %s', got_timeout)

        if (abs(got_timeout - self.faft_config.dev_screen_timeout) >
                self.TIMEOUT_MARGIN):
            raise error.TestFail(
                    'The developer firmware timeout does not match our spec: '
                    'expected %.2f +/- %.2f but got %.2f.' %
                    (self.faft_config.dev_screen_timeout, self.TIMEOUT_MARGIN,
                     got_timeout))

    def initialize(self, host, cmdline_args):
        super(firmware_DevScreenTimeout, self).initialize(host, cmdline_args)
        # This test is run on developer mode only.
        self.switcher.setup_mode('dev')
        self.setup_usbkey(usbkey=False)

    def run_once(self):
        if self.faft_config.fw_bypasser_type != 'ctrl_d_bypasser':
            raise error.TestNAError("This test is only valid on devices with "
                                    "screens.")

        logging.info("Always expected developer mode firmware A boot.")
        self.check_state((self.checkers.crossystem_checker, {
                              'devsw_boot': '1',
                              'mainfw_act': 'A',
                              'mainfw_type': 'developer',
                              }))

        logging.info("Reboot and press Ctrl-D repeatedly.")
        self.switcher.mode_aware_reboot(wait_for_dut_up=False)
        self.ctrl_d_repeatedly()
        self.switcher.wait_for_client()

        logging.info("Record the firmware boot time without waiting "
                     "firmware screen; on next reboot, do nothing and wait the "
                     "screen timeout.")
        self.record_fw_boot_time('ctrl_d_boot')
        self.switcher.mode_aware_reboot(wait_for_dut_up=False)
        self.switcher.wait_for_client()

        logging.info("Check the firmware screen timeout matches our spec.")
        self.check_timeout_period()
