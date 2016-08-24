# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import commands
import os
from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error

class build_RootFilesystemSize(test.test):
    """Test that we have a minimal amount of free space on rootfs."""
    version = 1


    PROD_IMAGE_ROOTFS_BYTES = '/root/bytes-rootfs-prod'


    def run_once(self):
        """Run the free space on rootfs test."""

        # Report the production size.
        if os.path.exists(self.PROD_IMAGE_ROOTFS_BYTES):
            rootfs_bytes_str = ''
            with open(self.PROD_IMAGE_ROOTFS_BYTES, 'r') as f:
                rootfs_bytes_str = f.read()
            if rootfs_bytes_str:
                self.output_perf_value('bytes_rootfs_prod',
                                       value=float(rootfs_bytes_str),
                                       units='bytes',
                                       higher_is_better=False)

        # Report the current (test) size.
        (status, output) = commands.getstatusoutput(
            'df -B1 --print-type / | tail -1')
        if status != 0:
            raise error.TestFail('Could not get size of rootfs')

        # Expected output format:
        # Filesystem     Type     1B-blocks      Used Available Use% Mounted on
        # /dev/root      ext2    1056858112 768479232 288378880  73% /
        output_columns = output.split()
        fs_type = output_columns[1]
        used = int(output_columns[3])
        free = int(output_columns[4])

        self.output_perf_value('bytes_rootfs_test', value=float(used),
                               units='bytes', higher_is_better=False)

        # Ignore squashfs as the free space will be reported as 0.
        if fs_type == 'squashfs':
            return

        # Fail if we are running out of free space on rootfs (20 MiB or
        # 2% free space).
        required_free_space = min(20 * 1024 * 1024, used * 0.02)

        if free < required_free_space:
            raise error.TestFail('%s bytes free is less than the %s required.' %
                                 (free, required_free_space))
