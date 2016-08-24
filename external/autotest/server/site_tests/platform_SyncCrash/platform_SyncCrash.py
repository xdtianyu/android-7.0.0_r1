# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import traceback
from autotest_lib.client.common_lib import error
from autotest_lib.server import test

class platform_SyncCrash(test.test):
    """Tests syncing to the file system before a crash.

    This test works in conjunction with testsync (found in
    punybench) to test various forms of sync. Testsync runs
    on the client to create a file, fill it with data, sync
    that data and crash the system.  The server side (this
    code), waits for the client to restart and then invokes
    testsync to verify the data did sync.

    Testsync has different phases for testing sync
    writer - syncs a file using fsync
    mapper - syncs a memory mapped file using msync
    verifier - verifies that content of the file

    The tests are run on both the stateful partition
    and ecryptfs mounted on a directory in the stateful
    partition.
    """
    version = 1
    _STATEFUL_DIR = '/usr/local/CrashDir/'
    _ECRYPT_DIR = '/usr/local/ecryptfs_tst/'
    _ECRYPT_MOUNT_POINT = '/usr/local/ecryptfs_mnt/'
    _ECRYPT_TEST_DIR = '%s/CrashDir/' % _ECRYPT_MOUNT_POINT
    _FILE = 'xyzzy'
    _Testsync = '/usr/local/opt/punybench/bin/testsync'


    def _run(self, cmd):
        """Run the given command and log results

        @param cmd: command to be run
        """
        result = self.client.run(cmd)
        if result.exit_status != 0:
            logging.error('%s: %s', cmd, result.stdout)
        else:
            logging.info('%s', cmd)


    def _testsync_crash(self, args):
        """Run testsync on the client

        Since testsync forces a crash of the system, the code
        waits for the reboot.

        @param args: arguments to pass testsync
        """
        logging.info('Crash: %s', self.client.hostname)
        try:
            cmd = '%s %s' % (self._Testsync, args)
            logging.info('Crash: %s', cmd)
            self.client.reboot(reboot_cmd=cmd)
        except error.AutoservRebootError as e:
            raise error.TestFail('%s.\nTest failed with error %s' % (
                    traceback.format_exc(), str(e)))


    def _testsync_verify(self, args):
        """Verify the results from previous testsync

        @param args: arguments to pass to testsync
        """
        cmd = '%s %s -p verifier' % (self._Testsync, args)
        result = self.client.run(cmd)
        if result.exit_status != 0:
            logging.error('%s: %s', cmd, result.stdout)


    def _ecrypt_setup(self):
        """Setup an eCrypt File System and mount it
        """
        edir = self._ECRYPT_DIR
        mnt = self._ECRYPT_MOUNT_POINT
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


    def _ecrypt_teardown(self):
        """Teardown the eCrypt File System
        """
        edir = self._ECRYPT_DIR
        mnt = self._ECRYPT_MOUNT_POINT
        self._run('umount %s' % edir)
        self._run('rm -R %s %s' % (edir, mnt))


    def _sync_stateful(self, dir, phase):
        """Crash the stateful file system while changing it

        @param dir - directory where test files are created
        @param phase - which phase of testsync to invoke
        """
        size = 0x1000
        file = dir + self._FILE
        self._run('mkdir -p %s' % dir)
        self._testsync_crash('-f %s -z %s -p %s' % (file, size, phase))
        self._testsync_verify('-f %s -z %s' % (file, size))
        self._run('rm -r %s' % dir)


    def _sync_ecryptfs(self, dir, phase):
        """Crash an ecryptfs file system right after a sync

        @param edir - directory used for the encrypted file system
        @param mnt - mount point for the encrypted file system
        @param dir - directory where test files are created
        @param phase - which phase of testsync to invoke
        """
        size = 0x1000
        file = dir + self._FILE
        self._ecrypt_setup()
        self._run('mkdir -p %s' % dir)
        self._testsync_crash('-f %s -z %s -p %s' % (file, size, phase))
        self._ecrypt_setup()
        self._testsync_verify('-f %s -z %s' % (file, size))
        self._run('rm -r %s' % dir)
        self._ecrypt_teardown()


    def run_once(self, host=None):
        """run_once runs the test.

        1. Runs a crash test on stateful partition
        2. Create an ecryptfs volume and run the same
           crash test

        @param host - the host machine running the test
        """
        self.client = host

        self._sync_stateful(self._STATEFUL_DIR, 'writer')
        self._sync_stateful(self._STATEFUL_DIR, 'mapper')

        self._sync_ecryptfs(self._ECRYPT_TEST_DIR, 'writer')
        self._sync_ecryptfs(self._ECRYPT_TEST_DIR, 'mapper')
