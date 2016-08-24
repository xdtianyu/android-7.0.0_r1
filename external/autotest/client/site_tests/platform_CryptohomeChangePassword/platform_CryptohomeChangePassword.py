# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os
import re
import shutil

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error, utils
from autotest_lib.client.cros import constants

class platform_CryptohomeChangePassword(test.test):
    version = 1

    def __run_cmd(self, cmd):
        result = utils.system_output(cmd + ' 2>&1', retain_output=True,
                                     ignore_status=True)
        return result


    def run_once(self):
        test_user = 'this_is_a_local_test_account@chromium.org';
        test_password = 'this_is_a_test_password';
        # Get the hash for the test user account
        cmd = ('/usr/sbin/cryptohome --action=obfuscate_user --user='
               + test_user)
        user_hash = self.__run_cmd(cmd).strip()

        # Remove the test user account
        cmd = ('/usr/sbin/cryptohome --action=remove --force --user='
               + test_user)
        self.__run_cmd(cmd)
        # Ensure that the user directory does not exist
        if os.path.exists(os.path.join(constants.SHADOW_ROOT, user_hash)):
          raise error.TestFail('Cryptohome could not remove the test user.')

        # Mount the test user account
        cmd = ('/usr/sbin/cryptohome --async --action=mount --user=' + test_user
               + ' --password=' + test_password + ' --create')
        self.__run_cmd(cmd)
        # Ensure that the user directory exists
        if not os.path.exists(os.path.join(constants.SHADOW_ROOT, user_hash)):
          raise error.TestFail('Cryptohome could not create the test user.')
        # Ensure that the user directory is mounted
        cmd = ('/usr/sbin/cryptohome --action=is_mounted')
        if (self.__run_cmd(cmd).strip() == 'false'):
          raise error.TestFail('Cryptohome created the user but did not mount.')

        # Unmount the directory
        cmd = ('/usr/sbin/cryptohome --action=unmount')
        self.__run_cmd(cmd)
        # Ensure that the user directory is not mounted
        cmd = ('/usr/sbin/cryptohome --action=is_mounted')
        if (self.__run_cmd(cmd).strip() != 'false'):
          raise error.TestFail('Cryptohome did not unmount the user.')

        # Try to migrate the password
        new_password = 'this_is_a_new_password'
        cmd = ('/usr/sbin/cryptohome --async --action=migrate_key --user='
               + test_user + ' --password=' + new_password + ' --old_password='
               + test_password)
        self.__run_cmd(cmd)

        # Mount the test user account with the new password
        cmd = ('/usr/sbin/cryptohome --async --action=mount --user=' + test_user
               + ' --password=' + new_password)
        self.__run_cmd(cmd)
        # Ensure that the user directory is mounted
        cmd = ('/usr/sbin/cryptohome --action=is_mounted')
        if (self.__run_cmd(cmd).strip() == 'false'):
          raise error.TestFail('Cryptohome did not mount with the new'
                               + ' password.')

        # Unmount the directory
        cmd = ('/usr/sbin/cryptohome --action=unmount')
        self.__run_cmd(cmd)
        # Ensure that the user directory is not mounted
        cmd = ('/usr/sbin/cryptohome --action=is_mounted')
        if (self.__run_cmd(cmd).strip() == 'true'):
          raise error.TestFail('Cryptohome did not unmount the user.')

        # Ensure the old password doesn't work
        cmd = ('/usr/sbin/cryptohome --async --action=mount --user=' + test_user
               + ' --password=' + test_password)
        self.__run_cmd(cmd)
        # Ensure that the user directory is not mounted
        cmd = ('/usr/sbin/cryptohome --action=is_mounted')
        if (self.__run_cmd(cmd).strip() != 'false'):
          raise error.TestFail('Cryptohome mounted with the old password.')

        # Remove the test user account
        cmd = ('/usr/sbin/cryptohome --action=remove --force --user='
               + test_user)
        self.__run_cmd(cmd)
