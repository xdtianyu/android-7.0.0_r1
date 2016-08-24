# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.server.cros.faft.firmware_test import FirmwareTest


class firmware_FWtries(FirmwareTest):
    """
    Boot with firmware B until fwb_tries/fw_try_count counts down to
    0.  vboot1 only needs to set fwb_tries in order to boot into FWB,
    but vboot2 needs to set two fields: fw_try_next and fw_try_count
    in order to do so.

    Setup Steps:
    1. Make the device in normal/dev mode.

    Test Steps:
    2. Set # of tries to 2 (through try_fwb)
      a.  For vboot1:
        set fwb_tries=2
        [fwb_tries can be > 0 and <= 15. Value will be auto reset to 15 If
        the value is < 0 or > 15
      b.  For vboot2:
        set fw_try_next=B fw_try_count=2
    3. Reboot 1
    4. Reboot 2
    5. Reboot 3

    Verification Steps:
    1. After reboot 1, fw_tries_checker checks that
    mainfw_act = B
    fwb_tries/fw_try_count = 1

    2. After reboot 2, fw_tries_checker checks that
    mainfw_act = B
    fwb_tries/fw_try_count = 0

    3. After reboot 3, fw_tries_checker
    mainfw_act = A
    fwb_tries/fw_try_count = 0
    """

    version = 1

    def initialize(self, host, cmdline_args, dev_mode=False):
        super(firmware_FWtries, self).initialize(host, cmdline_args)
        self.switcher.setup_mode('dev' if dev_mode else 'normal')

    def run_once(self, host):
        self.check_state((self.checkers.fw_tries_checker, ('A', True, 0)))

        self.try_fwb(2);

        self.check_state((self.checkers.fw_tries_checker, ('A', True, 2)))
        self.switcher.mode_aware_reboot()
        if self.faft_client.system.has_host():
            # Android: Does not have chromeos mechanism to block init file from
            # resetting try_count to 0 if in testing mode, so we just need to
            # check if we successfully booted into B
            self.check_state((self.checkers.fw_tries_checker, ('B', True, 0)))
            self.switcher.mode_aware_reboot()
            self.check_state((self.checkers.fw_tries_checker, ('B', True, 0)))
        else:
            # ChromeOS: Blocks init file on bootup from setting try_count to 0
            # Thus, each reboot is never successful, thus when try_count
            # decrements to 0, will reboot into FW A due to failure
            self.check_state((self.checkers.fw_tries_checker, ('B', True, 1)))
            self.switcher.mode_aware_reboot()
            self.check_state((self.checkers.fw_tries_checker, ('B', True, 0)))
            self.switcher.mode_aware_reboot()
            self.check_state((self.checkers.fw_tries_checker, ('A', True, 0)))
