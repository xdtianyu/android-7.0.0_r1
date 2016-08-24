# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import cryptohome


class platform_BootLockbox(test.test):
    """ Test basic boot-lockbox functionality."""
    version = 1

    def initialize(self):
        test.test.initialize(self)
        self.data_file = '/tmp/__lockbox_test'
        open(self.data_file, mode='w').write('test_lockbox_data')

    def cleanup(self):
        self._remove_file(self.data_file)
        self._remove_file(self.data_file + '.signature')
        self._remove_file('/var/lib/boot-lockbox/boot_attributes.pb')
        self._remove_file('/var/lib/boot-lockbox/boot_attributes.sig')
        test.test.cleanup(self)

    def _remove_file(self, filename):
        try:
            os.remove(filename)
        except OSError:
            # Ignore errors
            pass

    def _ensure_tpm_ready(self):
        status = cryptohome.get_tpm_status()
        if not status['Enabled']:
            raise error.TestNAError('Test NA because there is no TPM.')
        if not status['Owned']:
            cryptohome.take_tpm_ownership()
        status = cryptohome.get_tpm_status()
        if not status['Ready']:
            raise error.TestError('Failed to initialize TPM.')

    def _sign_lockbox(self):
        return utils.system(cryptohome.CRYPTOHOME_CMD +
                            ' --action=sign_lockbox --file=' + self.data_file,
                            ignore_status=True) == 0

    def _verify_lockbox(self):
        return utils.system(cryptohome.CRYPTOHOME_CMD +
                            ' --action=verify_lockbox --file=' + self.data_file,
                            ignore_status=True) == 0

    def _finalize_lockbox(self):
        utils.system(cryptohome.CRYPTOHOME_CMD + ' --action=finalize_lockbox')

    def _get_boot_attribute(self):
        return utils.system(cryptohome.CRYPTOHOME_CMD +
                            ' --action=get_boot_attribute --name=test',
                            ignore_status=True) == 0

    def _set_boot_attribute(self):
        utils.system(cryptohome.CRYPTOHOME_CMD +
                     ' --action=set_boot_attribute --name=test --value=1234')

    def _flush_and_sign_boot_attributes(self):
        return utils.system(cryptohome.CRYPTOHOME_CMD +
                            ' --action=flush_and_sign_boot_attributes',
                            ignore_status=True) == 0

    def run_once(self):
        self._ensure_tpm_ready()
        if not self._sign_lockbox():
            # This will fire if you forget to reboot before running the test!
            raise error.TestFail('Boot lockbox could not be signed.')

        if cryptohome.get_login_status()['boot_lockbox_finalized']:
            raise error.TestFail('Boot lockbox is already finalized.')

        if not self._verify_lockbox():
            raise error.TestFail('Boot lockbox could not be verified.')

        # Setup a bad signature and make sure it doesn't verify.
        open(self.data_file, mode='w').write('test_lockbox_data2')
        if self._verify_lockbox():
            raise error.TestFail('Boot lockbox verified bad data.')
        open(self.data_file, mode='w').write('test_lockbox_data')

        self._set_boot_attribute()

        if self._get_boot_attribute():
            raise error.TestFail('Boot attributes already have data.')

        if not self._flush_and_sign_boot_attributes():
            raise error.TestFail('Boot attributes could not sign.')

        if not self._get_boot_attribute():
            raise error.TestFail('Boot attribute was not available.')

        # Check again to make sure nothing has tricked the finalize check.
        if cryptohome.get_login_status()['boot_lockbox_finalized']:
            raise error.TestFail('Boot lockbox prematurely finalized.')

        # Finalize and make sure we can verify but not sign.
        self._finalize_lockbox()

        if not cryptohome.get_login_status()['boot_lockbox_finalized']:
            raise error.TestFail('Boot lockbox finalize status did not change '
                                 'after finalization.')

        if self._flush_and_sign_boot_attributes():
            raise error.TestFail('Boot attributes signed after finalization.')

        if not self._verify_lockbox():
            raise error.TestFail('Boot lockbox could not be verified after '
                                 'finalization.')

        if self._sign_lockbox():
            raise error.TestFail('Boot lockbox signed after finalization.')
