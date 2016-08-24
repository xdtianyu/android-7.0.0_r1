# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.common_lib import error
from autotest_lib.server.cros.faft.firmware_test import FirmwareTest


class firmware_TPMVersionCheck(FirmwareTest):
    """
    crossystem check of reported TPM version.

    Replacement for test '1.1.9 TPM_version_in_Crossystem [tcm:6762253]'.
    """
    version = 1

    def initialize(self, host, cmdline_args, dev_mode=False, ec_wp=None):
        super(firmware_TPMVersionCheck, self).initialize(host, cmdline_args,
                                                         ec_wp=ec_wp)
        self.switcher.setup_mode('dev' if dev_mode else 'normal')
        self.setup_usbkey(usbkey=False)

    def run_once(self):
        if not self.checkers.crossystem_checker({
                    'tpm_fwver': '0x00010001',
                    'tpm_kernver': '0x00010001', }):
            raise error.TestFail('tpm version keys reported by '
                                 'crossystem are not as expected.')
