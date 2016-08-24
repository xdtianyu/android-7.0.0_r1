# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, os, re, shutil, sys, time
from autotest_lib.client.bin import test, utils

class platform_CryptohomeTPMReOwn(test.test):
    """
    Test of cryptohome functionality to re-create a user's vault directory if
    the TPM is cleared and re-owned and the vault keyset is TPM-wrapped.
    """
    version = 1
    preserve_srcdir = True


    def __run_cmd(self, cmd):
        result = utils.system_output(cmd + ' 2>&1', retain_output=True,
                                     ignore_status=True)
        return result


    def run_once(self, subtest='None'):
        test_user = 'this_is_a_local_test_account@chromium.org'
        test_password = 'this_is_a_test_password'

        logging.info("Running client subtest %s", subtest)
        if (subtest == 'clear_tpm'):
            output = self.__run_cmd("/usr/sbin/tpm_clear --force")
            self.job.set_state("client_status", "Success")
        elif (subtest == 'enable_tpm'):
            output = self.__run_cmd("/usr/bin/tpm_init_temp_fix")
            self.job.set_state("client_status", "Success")
        elif (subtest == 'mount_cryptohome'):
            output = self.__run_cmd("/usr/sbin/cryptohome --action=remove " +
                                    "--force --user=" + test_user)
            ready = False
            for n in range(0, 20):
                output = self.__run_cmd("/usr/sbin/cryptohome " +
                                        "--action=tpm_status")
                if (output.find("TPM Ready: true") >= 0):
                    ready = True
                    break
                time.sleep(10)
            if (ready == False):
                error_msg = "TPM never became ready"
                self.job.set_state("client_status", error_msg)
                return
            output = self.__run_cmd("/usr/sbin/cryptohome --action=mount" +
                               " --user=" + test_user +
                               " --password=" + test_password)
            if (output.find("Mount succeeded") < 0):
                error_msg = "Cryptohome mount failed"
                self.job.set_state("client_status", error_msg)
                return
            output = self.__run_cmd("echo TEST_CONTENT > " +
                                    "/home/chronos/user/TESTFILE")
            output = self.__run_cmd("/usr/sbin/cryptohome --action=unmount")
            output = self.__run_cmd("/usr/sbin/cryptohome " +
                                    "--action=dump_keyset --user=" + test_user)
            if (output.find("TPM_WRAPPED") < 0):
                error_msg = 'Cryptohome did not create a TPM-wrapped keyset.'
                self.job.set_state("client_status", error_msg)
                return
            self.job.set_state("client_status", "Success")
        elif (subtest == 'mount_cryptohome_after_reboot'):
            ready = False
            for n in range(0, 20):
                output = self.__run_cmd("/usr/sbin/cryptohome " +
                                        "--action=tpm_status")
                if (output.find("TPM Ready: true") >= 0):
                    ready = True
                    break
                time.sleep(10)
            if (ready == False):
                error_msg = 'TPM never became ready'
                self.job.set_state("client_status", error_msg)
                return
            output = self.__run_cmd("/usr/sbin/cryptohome --action=mount" +
                               " --user=" + test_user +
                               " --password=" + test_password)
            if (output.find("Mount succeeded") < 0):
                error_msg = 'Cryptohome mount failed'
                self.job.set_state("client_status", error_msg)
                return
            output = self.__run_cmd("cat /home/chronos/user/TESTFILE 2>&1")
            if (output.find("TEST_CONTENT") < 0):
                output = self.__run_cmd("/usr/sbin/cryptohome --action=unmount")
                error_msg = ('Cryptohome did not contain original test file')
                self.job.set_state("client_status", error_msg)
                return
            output = self.__run_cmd("/usr/sbin/cryptohome --action=unmount")
            self.job.set_state("client_status", "Success")
        elif (subtest == 'mount_cryptohome_check_recreate'):
            ready = False
            for n in range(0, 20):
                output = self.__run_cmd("/usr/sbin/cryptohome " +
                                        "--action=tpm_status")
                if (output.find("TPM Ready: true") >= 0):
                    ready = True
                    break
                time.sleep(10)
            if (ready == False):
                error_msg = 'TPM never became ready'
                self.job.set_state("client_status", error_msg)
                return
            output = self.__run_cmd("/usr/sbin/cryptohome --action=mount" +
                               " --user=" + test_user +
                               " --password=" + test_password)
            if (output.find("Mount succeeded") < 0):
                error_msg = 'Cryptohome mount failed'
                self.job.set_state("client_status", error_msg)
                return
            output = self.__run_cmd("cat /home/chronos/user/TESTFILE 2>&1")
            if (output.find("TEST_CONTENT") >= 0):
                output = self.__run_cmd("/usr/sbin/cryptohome --action=unmount")
                error_msg = ('Cryptohome not re-created, ' +
                             'found original test file')
                self.job.set_state("client_status", error_msg)
                return
            output = self.__run_cmd("/usr/sbin/cryptohome --action=unmount")
            output = self.__run_cmd("/usr/sbin/cryptohome " +
                                    "--action=dump_keyset --user=" + test_user)
            if (output.find("TPM_WRAPPED") < 0):
                error_msg = ('Cryptohome did not create a ' +
                             'TPM-wrapped keyset on reboot.')
                self.job.set_state("client_status", error_msg)
                return
            self.job.set_state("client_status", "Success")
