# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os

from autotest_lib.server.cros.faft.firmware_test import FirmwareTest


class firmware_ShellBall(FirmwareTest):
    """
    chromeos-firmwareupdate functional tests.

    Checks the mode condition and enables or disables developement mode
    accordingly and runs all shellball functioanl tests.
    """
    version = 1

    _shellball_name = None

    def update_firmware(self, mode):
        self.faft_client.system.run_shell_command('%s --mode %s' %
                                                  (self._shellball_name, mode))
        # Enalbe dev mode if the mode is todev.
        if mode == 'todev':
            self.servo.enable_development_mode()
        # Disable dev mode if the mode is tonormal.
        elif mode == 'tonormal':
            self.servo.disable_development_mode()

    def install_original_firmware(self):
        self.faft_client.system.run_shell_command(
            'sudo chromeos-firmwareupdate --mode=factory_install')
        self.invalidate_firmware_setup()

    def initialize(self, host, cmdline_args, shellball_path=None,
                   shellball_name=None):
        super(firmware_ShellBall, self).initialize(host, cmdline_args)
        self._shellball_name = "/home/chronos/%s" % self._shellball_name
        host.send_file("%s/%s" % (shellball_path, shellball_name),
                       self._shellball_name)
        self.faft_client.system.run_shell_command('chmod +x %s' %
                                                  self._shellball_name)
        self.switcher.setup_mode('normal')
        # Get crossystem fwid.
        [self._current_fwid] = (
            self.faft_client.system.run_shell_command_get_output(
                'crossystem fwid'))
        # Get BIOS version from shellball.
        [self._shellball_fwid] = self.faft_client. \
                                        system.run_shell_command_get_output(
                                            '%s -V | grep "BIOS version"'
                                            ' | sed "s/BIOS version: '
                                            '\(.*\)/\\1/" '
                                            % self._shellball_name)

    def cleanup(self):
        if os.path.exists(self._shellball_name):
            os.remove(self._shellball_name)
        super(firmware_ShellBall, self).cleanup()

    def run_once(self):
        logging.info("Change to devmode.")
        self.check_state((self.checkers.crossystem_checker,
                          {'dev_boot_usb': '0'}))
        self.update_firmware('todev')
        self.switcher.mode_aware_reboot()

        logging.info("Check mainfw_type and run autoupdate.")
        self.check_state((self.checkers.crossystem_checker,
                          {'mainfw_type': 'developer'}))
        self.update_firmware('autoupdate')
        self.switcher.mode_aware_reboot()

        logging.info("Verify fwid and install system firmware.")
        self.check_state((self.checkers.crossystem_checker,
                          {'fwid': self._shellball_fwid}))
        self.install_original_firmware()
        self.switcher.mode_aware_reboot()

        logging.info("Verify the old firmware id and test factory_install.")
        self.check_state((self.checkers.crossystem_checker,
                          {'fwid': self._current_fwid}))
        self.update_firmware('factory_install')
        self.switcher.mode_aware_reboot()

        logging.info("Verify fwid and install original firmware.")
        self.check_state((self.checkers.crossystem_checker,
                          {'fwid': self._shellball_fwid}))
        self.install_original_firmware()
        self.switcher.mode_aware_reboot()

        logging.info("Verify old fwid.")
        self.check_state((self.checkers.crossystem_checker,
                          {'fwid': self._current_fwid}))
