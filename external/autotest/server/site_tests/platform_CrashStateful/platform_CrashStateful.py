# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import traceback
from autotest_lib.client.common_lib import error
from autotest_lib.server import test

class platform_CrashStateful(test.test):
    """Tests the crash recovery of the stateful file system

    1. Create a specific file 'charlie'
    2. Sync
    3. Crash system
    4. Wait for reboot
    5. Check if 'charlie' is there and complete
    6. Clean up

    Do the samething with an ecryptfs volume.
    """
    version = 1
    _STATEFUL_DIR = '/usr/local/CrashDir'
    _ECRYPT_DIR = '/usr/local/ecryptfs_tst'
    _ECRYPT_MOUNT_POINT = '/usr/local/ecryptfs_mnt'
    _ECRYPT_TEST_DIR = '%s/CrashDir' % _ECRYPT_MOUNT_POINT


    def _run(self, cmd):
        """Run the give command and log results

        @param cmd: command to be run
        """
        result = self.client.run(cmd)
        if result.exit_status != 0:
            logging.error('%s: %s', cmd, result.stdout)


    def _ecrypt_mount(self, edir, mnt):
        """Mount the eCrypt File System

        @param ddir: directory where encrypted file system is stored
        @param mnt: mount point for encrypted file system
        """
        options = ('-o'
                   ' key=passphrase:passphrase_passwd=secret'
                   ',ecryptfs_cipher=aes'
                   ',ecryptfs_key_bytes=32'
                   ',no_sig_cache'
                   ',ecryptfs_passthrough=no'
                   ',ecryptfs_enable_filename_crypto=no')
        self._run('mkdir -p %s %s' % (edir, mnt))
        self._run('mount -t ecryptfs %s %s %s' %
                           (options, edir, mnt))


    def _ecrypt_unmount(self, edir, mnt):
        """Unmount the eCrypt File System and remove it and its mount point

        @param dir: directory where encrypted file system is stored
        @param mnt: mount point for encrypted file system
        """
        self._run('umount %s' % edir)
        self._run('rm -R %s' % edir)
        self._run('rm -R %s' % mnt)


    def _crash(self):
        """crash the client without giving anything a chance to clean up

        We use the kernel crash testing interface to immediately reboot the
        system. No chance for any flushing of I/O or cleaning up.
        """
        logging.info('CrashStateful: force panic %s', self.client.hostname)
        interface = "/sys/kernel/debug/provoke-crash/DIRECT"
        cmd = 'echo PANIC > %s' % interface
        if not self.client.run('ls %s' % interface,
                               ignore_status=True).exit_status == 0:
            interface = "/proc/breakme"
            cmd = 'echo panic > %s' % interface
        try:
            """The following is necessary to avoid command execution errors
            1) If ssh on the DUT doesn't terminate cleanly, it will exit with
               status 255 causing an exception
            2) ssh won't terminate if a background process holds open stdin,
               stdout, or stderr
            3) without a sleep delay, the reboot may close the connection with
               an error
            """
            wrapped_cmd = 'sleep 1; %s'
            self.client.reboot(reboot_cmd=wrapped_cmd % cmd)
        except error.AutoservRebootError as e:
            raise error.TestFail('%s.\nTest failed with error %s' % (
                    traceback.format_exc(), str(e)))


    def _create_file_and_crash(self, dir):
        """Sets up first part of test, then crash

        @param dir - directory where test files are created
        """
        self._run('mkdir -p %s' % dir)
        self._run('echo charlie smith >%s/charlie' % dir)
        self._run('sync')
        self._crash()


    def _verify_and_cleanup(self, dir):
        """Verify results and clean up

        @param dir - directory where test files were created
        """
        result = self.client.run('cat %s/charlie' % dir)
        hi = result.stdout.strip()
        if hi != 'charlie smith':
            raise error.TestFail('Test failed, Sync mechanism failed')
        self._run('rm -fr %s' % dir)


    def _crash_stateful(self, dir):
        """Crash the stateful file system while changing it

        @param dir - directory where test files are created
        """
        self._create_file_and_crash(dir)
        self._verify_and_cleanup(dir)


    def _crash_ecrptfs(self, edir, mnt, dir):
        """Crash the stateful file system while changing it

        @param edir - directory used for the encrypted file system
        @param mnt - mount point for the encrypted file system
        @param dir - directory where test files are created
        """
        self._ecrypt_mount(edir, mnt)
        self._create_file_and_crash(dir)
        self._ecrypt_mount(edir, mnt)
        self._verify_and_cleanup(dir)
        self._ecrypt_unmount(edir, mnt)


    def run_once(self, host=None):
        """run_once runs the test.

        1. Runs a crash test on stateful partition
        2. Create an ecryptfs volume and run the same
           crash test

        @param host - the host machine running the test
        """
        self.client = host

        self._crash_stateful(self._STATEFUL_DIR)

        self._crash_ecrptfs(self._ECRYPT_DIR, self._ECRYPT_MOUNT_POINT,
            self._ECRYPT_TEST_DIR)
