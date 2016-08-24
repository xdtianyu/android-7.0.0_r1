# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, os
from autotest_lib.client.bin import site_utils, test, utils
from autotest_lib.client.common_lib import error


class hardware_StorageTrim(test.test):
    """
    Measure write performance before and after trim.

    Use fio to measure write performance.
    Use mkfs.ext4 to trim device.
    """

    version = 1


    def run_once(self, dev='/dev/sda'):
        """
        Measure write performance before and after trim.

        This test use an entire disk so we need to boot from usb.

        @param dev: block device to test
        """
        logging.info('Target device: %s', dev)

        # Check that device exist.
        if not os.path.exists(dev):
            msg = 'Test failed with error: %s not exist' % dev
            raise error.TestFail(msg)

        # Check that device is not rootdev.
        rootdev = site_utils.get_root_device()
        if dev == rootdev:
            raise error.TestFail('Can not test on root device')

        # Use fio to fill device first.
        self.job.run_test('hardware_StorageFio',
                          disable_sysinfo=True,
                          dev=dev,
                          filesize=0,
                          requirements=[('disk_fill', [])],
                          tag='disk_fill')

        # Use 4k random write with queue depth = 32 because manufacture usually
        # uses this use case in the SSD specification.
        # Also, print result every minute to look at the performance drop trend
        # over time. Result reported by autotest will be the last minute one.
        requirements = [('4k_write_qd32', ['--status-interval=60'])]

        # Check write performance
        self.job.run_test('hardware_StorageFio',
                          disable_sysinfo=True,
                          dev=dev,
                          filesize=0,
                          requirements=requirements,
                          tag='before_trim')

        # Unmount drive to make it possible to format.
        utils.run('umount %s*' % dev, ignore_status=True)
        # Format whole drive to ext4. Mkfs will trim the drive before format.
        utils.run('mkfs.ext4 -F %s' % dev, ignore_status=True)

        # Check write performance
        self.job.run_test('hardware_StorageFio',
                          disable_sysinfo=True,
                          dev=dev,
                          filesize=0,
                          requirements=requirements,
                          tag='after_trim')

