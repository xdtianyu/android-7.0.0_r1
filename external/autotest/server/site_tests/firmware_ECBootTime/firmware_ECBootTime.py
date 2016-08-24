# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.client.common_lib import error
from autotest_lib.server.cros.faft.firmware_test import FirmwareTest


class firmware_ECBootTime(FirmwareTest):
    """
    Servo based EC boot time test.
    """
    version = 1

    def initialize(self, host, cmdline_args):
        super(firmware_ECBootTime, self).initialize(host, cmdline_args)
        # Only run in normal mode
        self.switcher.setup_mode('normal')

    def check_boot_time(self):
        """Check EC and AP boot times"""
        # Initialize a list of two strings, one printed by the EC when the AP
        # is taken out of reset, and another one printed when the EC observes
        # the AP running. These strings are used as for console output anchors
        # when calculating the AP boot time.
        #
        # This is very approximate, a better long term solution would be to
        # have the EC print the same fixed strings for these two events on all
        # platforms. http://crosbug.com/p/21628 has been opened to track this
        # issue.
        if self._x86:
            boot_anchors = ["\[([0-9\.]+) PB", "\[([0-9\.]+) Port 80"]
        elif self._arm_legacy:
            boot_anchors = ["\[([0-9\.]+) AP running ...",
                            "\[([0-9\.]+) XPSHOLD seen"]
        else:
            boot_anchors = ["\[([0-9\.]+) power state 1 = S5",
                            "\[([0-9\.]+) power state 3 = S0"]
        # regular expression to say that EC is ready. For systems that
        # run out of ram there is a second boot where the PMIC is
        # asked to power cycle the EC to be 100% sure (I wish) that
        # the code is clean. Looking for the "Inits done" generates a
        # match after the first boot, and introduces a race between
        # the EC booting the second time and the test sending the
        # power_cmd.
        if self._doubleboot:
            ec_ready = ["\[([0-9.]+) power state 0"]
        else:
            ec_ready = ["([0-9.]+) Inits done"]
        power_cmd = "powerbtn"  if self._x86 or self._ryu else "power on"
        reboot = self.ec.send_command_get_output(
            "reboot ap-off", ec_ready)
        power_press = self.ec.send_command_get_output(
            power_cmd, boot_anchors)
        reboot_time = float(reboot[0][1])
        power_press_time = float(power_press[0][1])
        firmware_resp_time = float(power_press[1][1])
        boot_time = firmware_resp_time - power_press_time
        logging.info("EC cold boot time: %f s", reboot_time)
        if reboot_time > 1.0:
            raise error.TestFail("EC cold boot time longer than 1 second.")
        logging.info("EC boot time: %f s", boot_time)
        if boot_time > 1.0:
            raise error.TestFail("Boot time longer than 1 second.")

    def is_arm_legacy_board(self):
        arm_legacy = ('Snow', 'Spring', 'Pit', 'Pi', 'Big', 'Blaze', 'Kitty')
        output = self.faft_client.system.get_platform_name()
        return output in arm_legacy

    def is_ryu_board(self):
        output = self.faft_client.system.get_platform_name()
        return output == 'Ryu'

    def run_once(self):
        if not self.check_ec_capability():
            raise error.TestNAError("Nothing needs to be tested on this device")
        self._x86 = ('x86' in self.faft_config.ec_capability)
        self._doubleboot = ('doubleboot' in self.faft_config.ec_capability)
        self._arm_legacy = self.is_arm_legacy_board()
        self._ryu = self.is_ryu_board()
        dev_mode = self.checkers.crossystem_checker({'devsw_boot': '1'})
        logging.info("Reboot and check EC cold boot time and host boot time.")
        self.switcher.mode_aware_reboot('custom', self.check_boot_time)

    def cleanup(self):
        # Restore the ec_uart_regexp to None
        self.ec.set_uart_regexp('None')
