# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.server.cros.faft.firmware_test import FirmwareTest


class firmware_TPMKernelVersion(FirmwareTest):
    """Dev mode will not corrupt kernel and firmware version stored in the TPM.

    Automate 1.2.4 Check Kernel version in TPM is not corrupted.
    """
    version = 1

    def initialize(self, host, cmdline_args):
        super(firmware_TPMKernelVersion, self).initialize(host, cmdline_args)
        self.switcher.setup_mode('dev')
        self.setup_usbkey(usbkey=True, host=False)
        self.original_dev_boot_usb = self.faft_client.system.get_dev_boot_usb()
        logging.info('Original dev_boot_usb value: %s',
                     str(self.original_dev_boot_usb))


    def dut_run_cmd(self, command):
        """Execute command on DUT.

        @param command: shell command to be executed on DUT.
        @returns command output.
        """
        logging.info('Execute %s', command)
        output = self.faft_client.system.run_shell_command_get_output(command)
        logging.info('Output %s', output)
        return output

    def cleanup(self):
        """Reboot device from SSD."""
        if self.faft_client.system.is_removable_device_boot():
          logging.info('Reboot into internal disk...')
          self.faft_client.system.set_dev_boot_usb(self.original_dev_boot_usb)
          self.switcher.mode_aware_reboot()
        super(firmware_TPMKernelVersion, self).cleanup()

    def run_once(self):
        # tcsd already shutdown by test framework.

        # Should return 1 4c 57 52 47 1 0 1 0 0 0 0 0
        command = 'tpmc read 0x1008 0x0d'
        self.dut_run_cmd(command)

        self.dut_run_cmd('crossystem kernkey_vfy dev_boot_usb')
        # Boot CrOS from USB
        self.faft_client.system.set_dev_boot_usb(1)
        self.switcher.mode_aware_reboot(wait_for_dut_up=False)
        self.switcher.bypass_dev_boot_usb()
        self.switcher.wait_for_client()

        # Check that DUT is booted from USB.
        self.check_state((self.checkers.crossystem_checker,
                          {'kernkey_vfy': 'hash'}))

        out = self.dut_run_cmd('crossystem tpm_kernver tpm_fwver')
        (kernver, fwver) = out[0].split(' ')
        logging.info('tpm_kernver=%s tpm_fwver=%s', kernver, fwver)
        assert kernver != '0xFFFFFFFF'
        assert fwver != '0xFFFFFFFF'
