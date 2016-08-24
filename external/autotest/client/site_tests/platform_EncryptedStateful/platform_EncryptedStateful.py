# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, os, tempfile, shutil, stat, time, posix
from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error

# TODO:
#  - mock out TPM and check all error conditions
#  - test failure when things aren't mounted correctly

class test_checker(object):
    def __init__(self):
        logging.info("test_checker.__init__")
        # Empty failure list means test passes.
        self._failures = []

    def _passed(self, msg):
        logging.info('ok: %s' % (msg))

    def _failed(self, msg):
        logging.error('FAIL: %s' % (msg))
        self._failures.append(msg)

    def _fatal(self, msg):
        logging.error('FATAL: %s' % (msg))
        raise error.TestError(msg)

    def check(self, boolean, msg, fatal=False):
        if boolean == True:
            self._passed(msg)
        else:
            msg = "could not satisfy '%s'" % (msg)
            if fatal:
                self._fatal(msg)
            else:
                self._failed(msg)

    def test_raise(self):
        # Raise a failure if anything unexpected was seen.
        if len(self._failures):
            raise error.TestFail((", ".join(self._failures)))

chk = test_checker()


class EncryptedStateful(object):
    def _prepare_simulated_root(self):
        os.makedirs(self.var)
        os.makedirs(self.chronos)
        os.makedirs(self.stateful)

        # Build fake stateful block device (emulate 10G sda1).
        self.stateful_block = os.path.join(self.root, 'stateful.block')
        utils.system("truncate -s 10G %s" % (self.stateful_block))
        utils.system("mkfs.ext4 -F %s" % (self.stateful_block))
        utils.system("mount -n -t ext4 -o loop,noatime,commit=600 %s %s" %
                     (self.stateful_block, self.stateful))

    def __init__(self, root=None):
        if root == None:
            self.root = tempfile.mkdtemp(dir='/mnt/stateful_partition',
                                         prefix='.test-enc-stateful-')
            self.simulated = True
        else:
            self.root = root
            self.simulated = False

        self.var = os.path.join(self.root, 'var')
        self.chronos = os.path.join(self.root, 'home', 'chronos')
        self.stateful = os.path.join(self.root, 'mnt', 'stateful_partition')
        self.mount_log = os.path.join(self.stateful, 'mount.log')
        self.key = os.path.join(self.stateful, 'encrypted.key')
        self.needs_finalization = os.path.join(self.stateful,
                                               'encrypted.needs-finalization')
        self.block = os.path.join(self.stateful, 'encrypted.block')
        self.encrypted = os.path.join(self.stateful, 'encrypted')

        if self.simulated:
            try:
                self._prepare_simulated_root()
            except:
                shutil.rmtree(self.root)
                raise

        self.mounted = not self.simulated

    def mount(self, args=""):
        if self.mounted or not self.simulated:
            return
        # TODO(keescook): figure out what is killing the resizer and
        # remove the explicit use of "tee" here.
        # Without the pipe to "tee", mount-encrypted's forked resizing
        # process gets killed, even though it is using daemon(). (Is
        # autotest doing something odd here?) This leaves the filesystem
        # unresized. It would be better to have the resizer running in
        # the background, as it is designed, so we can examine its behavior
        # during testing (e.g. "does the filesystem actually grow?").
        utils.system("MOUNT_ENCRYPTED_ROOT=%s mount-encrypted %s 2>&1 "
                     "| tee %s" % (self.root, args, self.mount_log))
        self.mounted = True

    def umount(self):
        if not self.mounted or not self.simulated:
            return
        utils.system("MOUNT_ENCRYPTED_ROOT=%s mount-encrypted umount" %
                         (self.root))
        self.mounted = False

    # Clean up when destroyed.
    def __del__(self):
        if self.simulated:
            self.umount()
            utils.system("umount -n %s" % (self.stateful))
            shutil.rmtree(self.root)

    # Perform common post-mount size/owner checks on the filesystem and
    # backing files.
    def check_sizes(self, finalized=True):
        # Do we have the expected backing files?
        chk.check(os.path.exists(self.block), "%s exists" % (self.block))
        if finalized:
            keyfile = self.key
            other = self.needs_finalization
        else:
            keyfile = self.needs_finalization
            other = self.key
        chk.check(os.path.exists(keyfile), "%s exists" % (keyfile))
        chk.check(not os.path.exists(other), "%s does not exist" % (other))

        # Sanity check the key file stat.
        info = os.stat(keyfile)
        chk.check(stat.S_ISREG(info.st_mode),
                  "%s is regular file" % (keyfile))
        chk.check(info.st_uid == 0, "%s is owned by root" % (keyfile))
        chk.check(info.st_gid == 0, "%s has group root" % (keyfile))
        chk.check(stat.S_IMODE(info.st_mode) == (stat.S_IRUSR | stat.S_IWUSR),
                  "%s is S_IRUSR | S_IWUSR" % (keyfile))
        chk.check(info.st_size == 48, "%s is 48 bytes" % (keyfile))

        # Sanity check the block file stat.
        info = os.stat(self.block)
        chk.check(stat.S_ISREG(info.st_mode),
                  "%s is regular file" % (self.block))
        chk.check(info.st_uid == 0, "%s is owned by root" % (self.block))
        chk.check(info.st_gid == 0, "%s has group root" % (self.block))
        chk.check(stat.S_IMODE(info.st_mode) == (stat.S_IRUSR | stat.S_IWUSR),
                  "%s is S_IRUSR | S_IWUSR" % (self.block))
        # Make sure block file is roughly a third of the size of the root
        # filesystem (within 5%).
        top = os.statvfs(self.stateful)
        backing_size = float(info.st_size)
        third = top.f_blocks * top.f_frsize * .3
        chk.check(backing_size > (third * .95)
                  and backing_size < (third * 1.05),
                  "%s is near %d bytes (was %d)" % (self.block, third,
                                                    info.st_size))

        # Wait for resize manager task to finish.
        utils.poll_for_condition(lambda: utils.system("pgrep mount-encrypted",
                                                      ignore_status=True) != 0,
                                 error.TestError('resizer still running'))

        # Verify filesystem is within 5% of the block file size.
        info = os.statvfs(self.encrypted)
        encrypted_size = float(info.f_frsize) * float(info.f_blocks)
        chk.check(encrypted_size / backing_size > 0.95,
                  "%s fs (%d) is nearly the backing device size (%d)" %
                  (self.encrypted, encrypted_size, backing_size))
        # Verify there is a reasonable number of inodes in the encrypted
        # filesystem (near 25% inodes-to-blocks ratio).
        inode_ratio = float(info.f_files) / float(info.f_blocks)
        chk.check(inode_ratio > 0.20 and inode_ratio < 0.30,
                  "%s has close to 25%% ratio of inodes-to-blocks (%.2f%%)" %
                  (self.encrypted, inode_ratio*100))

        # Raise non-fatal failures now, if they were encountered.
        chk.test_raise()

    # Wait for kernel background writing to finish.
    def _backing_stabilize(self):
        start = None
        size = 0
        while True:
            k = long(utils.system_output("du -sk %s" % (self.block),
                                         retain_output = True).split()[0])
            if start == None:
                start = k
            if k == size:
                # Backing file has remained the same size for 10 seconds.
                # Assume the kernel is done with background initialization.
                break
            time.sleep(10)
            utils.system("sync")
            size = k
        logging.info("%s stabilized at %dK (was %dK)" %
                     (self.block, size, start))

    # Check that the backing file reclaims space when filesystem contents
    # are deleted.
    def check_reclamation(self):
        # This test is sensitive to other things happening on the filesystem,
        # so we must wait for background initialization to finish first.
        self._backing_stabilize()

        megs = 200
        data = os.path.join(self.var, "check_reclamation")
        orig = os.statvfs(self.stateful)

        # 200M file added to encrypted filesystem.
        utils.system("dd if=/dev/zero of=%s bs=1M count=%s; sync" % (data,
                                                                     megs))
        # Wait for background allocations to finish.
        self._backing_stabilize()
        filled = os.statvfs(self.stateful)

        # 200M file removed from encrypted filesystem.
        utils.system("rm %s; sync" % (data))
        # Wait for background hole-punching to finish.
        self._backing_stabilize()
        done = os.statvfs(self.stateful)

        # Did the underlying filesystem grow by the size of the test file?
        file_blocks_used = float((megs * 1024 * 1024) / orig.f_frsize)
        fs_blocks_used = float(orig.f_bfree - filled.f_bfree)
        chk.check(file_blocks_used / fs_blocks_used > 0.95,
                  "%d file blocks account for most of %d fs blocks" %
                  (file_blocks_used, fs_blocks_used))

        # Did the underlying filesystem shrink on removal?
        fs_blocks_done = float(orig.f_bfree - done.f_bfree)
        chk.check(fs_blocks_done / file_blocks_used < 0.05,
                  "most of %d fs blocks reclaimed (%d fs blocks left over, "
                  "free: %d -> %d -> %d)" %
                  (fs_blocks_used, fs_blocks_done,
                   orig.f_bfree, filled.f_bfree, done.f_bfree))

        # Raise non-fatal failures now, if they were encountered.
        chk.test_raise()


class platform_EncryptedStateful(test.test):
    version = 1

    def existing_partition(self):
        # Examine the existing encrypted partition.
        encstate = EncryptedStateful("/")

        # Perform post-mount sanity checks (and handle unfinalized devices).
        encstate.check_sizes(finalized=os.path.exists(encstate.key))

    def factory_key(self):
        # Create test root directory.
        encstate = EncryptedStateful()

        # Make sure we haven't run here before.
        chk.check(not os.path.exists(encstate.key),
                  "%s does not exist" % (encstate.key))
        chk.check(not os.path.exists(encstate.block),
                  "%s does not exist" % (encstate.block))

        # Mount a fresh encrypted stateful, with factory static key.
        encstate.mount("factory")

        # Perform post-mount sanity checks.
        encstate.check_sizes()

        # Check disk reclamation.
        encstate.check_reclamation()

        # Check explicit umount.
        encstate.umount()

    def no_tpm(self):
        encstate = EncryptedStateful()

        # Relocate the TPM device during mount.
        tpm = "/dev/tpm0"
        off = "%s.off" % (tpm)
        try:
            if os.path.exists(tpm):
                utils.system("mv %s %s" % (tpm, off))
            # Mount without a TPM.
            encstate.mount()
        finally:
            if os.path.exists(off):
                utils.system("mv %s %s" % (off, tpm))

        # Perform post-mount sanity checks.
        encstate.check_sizes(finalized=False)

    def run_once(self):
        # Do a no-write test of system's existing encrypted partition.
        self.existing_partition()

        # Do a no-write, no-TPM test with sanity checks.
        self.no_tpm()

        # There is no interactively controllable TPM mock yet for
        # mount-encrypted, so we can only test the static key currently.
        self.factory_key()
