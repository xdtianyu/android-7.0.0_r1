# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import hashlib, logging, os

from autotest_lib.client.common_lib import error
from autotest_lib.server.cros.faft.firmware_test import FirmwareTest


class firmware_TPMExtend(FirmwareTest):
    """Test to ensure TPM PCRs are extended correctly."""
    version = 1

    def initialize(self, host, cmdline_args):
        super(firmware_TPMExtend, self).initialize(host, cmdline_args)
        self.switcher.setup_mode('normal')
        self.setup_usbkey(usbkey=True, host=False)

    def _check_pcr(self, num, hash_obj):
        """Returns true iff PCR |num| was extended with hashlib |hash_obj|."""
        pcrs_file='/sys/class/misc/tpm0/device/pcrs'
        if not os.path.exists(pcrs_file):
            pcrs_file='/sys/class/tpm/tpm0/device/pcrs'
        pcrs = '\n'.join(self.faft_client.system.run_shell_command_get_output(
                        'cat %s' % pcrs_file))
        logging.debug('Dumping PCRs read from device: \n%s', pcrs)
        extended = hashlib.sha1('\0' * 20 + hash_obj.digest()[:20]).hexdigest()
        spaced = ' '.join(extended[i:i+2] for i in xrange(0, len(extended), 2))
        logging.debug('PCR %d should contain hash: %s', num, spaced)
        return ('PCR-%.2d: %s' % (num, spaced.upper())) in pcrs

    def run_once(self):
        logging.info('Verifying HWID digest in PCR1')
        hwid = self.faft_client.system.run_shell_command_get_output(
                'crossystem hwid')[0]
        logging.debug('HWID reported by device is: %s', hwid)
        if not self._check_pcr(1, hashlib.sha256(hwid)):
            error.TestFail('PCR1 was not extended with SHA256 digest of HWID!')

        logging.info('Verifying bootmode digest in PCR0 in normal mode')
        self.check_state((self.checkers.crossystem_checker, {
                            'devsw_boot': '0',
                            'mainfw_type': 'normal'
                            }))
        # dev_mode: 0, rec_mode: 0, keyblock_flags: "normal" (1)
        if not self._check_pcr(0, hashlib.sha1(chr(0) + chr(0) + chr(1))):
            error.TestFail('PCR0 was not extended with bootmode 0|0|1!')

        logging.info('Verifying bootmode digest in PCR0 in recovery mode')
        self.switcher.reboot_to_mode(to_mode='rec')
        self.check_state((self.checkers.crossystem_checker, {
                            'devsw_boot': '0',
                            'mainfw_type': 'recovery'
                            }))
        # dev_mode: 0, rec_mode: 1, keyblock_flags: "unknown" (0)
        if not self._check_pcr(0, hashlib.sha1(chr(0) + chr(1) + chr(0))):
            error.TestFail('PCR0 was not extended with bootmode 0|1|0!')

        logging.info('Transitioning to dev mode for next test')
        self.switcher.reboot_to_mode(to_mode='dev')

        logging.info('Verifying bootmode digest in PCR0 in developer mode')
        self.check_state((self.checkers.crossystem_checker, {
                            'devsw_boot': '1',
                            'mainfw_type': 'developer'
                            }))
        # dev_mode: 1, rec_mode: 0, keyblock_flags: "normal" (1)
        if not self._check_pcr(0, hashlib.sha1(chr(1) + chr(0) + chr(1))):
            error.TestFail('PCR0 was not extended with bootmode 1|0|1!')

        logging.info('Verifying bootmode digest in PCR0 in dev-recovery mode')
        self.switcher.reboot_to_mode(to_mode='rec')
        self.check_state((self.checkers.crossystem_checker, {
                            'devsw_boot': '1',
                            'mainfw_type': 'recovery'
                            }))
        # dev_mode: 1, rec_mode: 1, keyblock_flags: "unknown" (0)
        if not self._check_pcr(0, hashlib.sha1(chr(1) + chr(1) + chr(0))):
            error.TestFail('PCR0 was not extended with bootmode 1|1|0!')

        logging.info('All done, returning to normal mode')
        self.switcher.reboot_to_mode(to_mode='normal')
