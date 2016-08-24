# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

__author__ = 'ups@chromium.org (Stephan Uphoff)'

import logging
import os
import utils

from autotest_lib.client.bin import utils, test
from autotest_lib.client.common_lib import error


class security_ChromiumOSLSM(test.test):
    """
    Verify Chromium OS Security Module behaves as expected.
    """
    version = 1

    def _mount(self, target):
        cmd = "mount -c -n -t tmpfs -o nodev,noexec,nosuid test %s" % (target)
        return utils.system(cmd, ignore_status=True)

    def _umount(self, target):
        utils.system('umount -n %s' % (target))

    def _check_mount(self, target, expected, msg):
        succeeded = (self._mount(target) == 0)
        if succeeded:
            self._umount(target)
        if succeeded != expected:
            logging.error(msg)
            return 1
        return 0

    def run_once(self):
        errors = 0
        test_dir = '/tmp/chromium_lsm_test_dir'
        os.mkdir(test_dir, 0700)

        mnt_target = '%s/mount_point' % (test_dir)
        os.mkdir(mnt_target, 0700)

        sym_target = '%s/symlink' % (test_dir)
        os.symlink('mount_point', sym_target)

        # Mounting should succeed (no symbolic link in mount path).
        errors += self._check_mount(mnt_target, True,
                                    'Unable to mount on a directory')

        # Mounting should fail as we used a mount path with a symbolic link.
        errors += self._check_mount(sym_target, False,
                                    'Unexpectedly mounted on a symlink')

        utils.system('rm -rf ' + test_dir)
        # If self.error is not zero, there were errors.
        if errors > 0:
            raise error.TestFail('Failed %d tests' % errors)
