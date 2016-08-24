# Copyright (c) 2009 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import re
import traceback
from autotest_lib.client.common_lib import error
from autotest_lib.server import test

class platform_CorruptRootfs(test.test):
    """Tests how the system recovers when the root file system is corrupted

    1. Copies kernel A and rootfs A to kernel B and rootfs B.
    2. Sets the modes on the partitions.
    3. Corrupts rootfs A by writing some random data to a the first few sectors.
    4. If enabled, corrupts the bootcache area.
    5. Reboots the system.
    6. Runs the test again in the reverse direction, leaving kernel A and
       rootfs A in the same state.
    """

    version = 1


    def _get_bootcache_offset(self):
        """Gets the offset of the bootcache from the command line.

        @return
          if bootcache is found, returns offset as a string
          otherwise returns '0'
        """

        # Get the linux cmd line
        result = self.client.run('cat /proc/cmdline')

        m = re.search('dm="(.*)"', result.stdout)
        dm = m.group(1)
        i = dm.find('bootcache')
        if i > 0:
            s = dm[i:].split()
            return s[2]   # 2nd field after bootcache has sector offset
        return '0'


    def _get_partition_layout(self):
        """Get the partition layout

        @return
          dev - name of the device hosting the partition
          kernelA - partition used to boot kernel
          rootfsA - partition of current root file system
          kernelB - backup copy of kernel
          rootfsB - backup copy of root file system
        """

        # What is our root partition?
        # TODO(crbug.com/226082)
        result = self.client.run('rootdev -s')
        logging.info('Root partition %s', result.stdout)
        rootdev = result.stdout.strip()

        if rootdev == '/dev/sda3':
            dev = '/dev/sda'
            kernelA = dev + '2'
            rootfsA = dev + '3'
            kernelB = dev + '4'
            rootfsB = dev + '5'
        elif rootdev == '/dev/sda5':
            dev = '/dev/sda'
            kernelA = dev + '4'
            rootfsA = dev + '5'
            kernelB = dev + '2'
            rootfsB = dev + '3'
        elif rootdev == '/dev/mmcblk0p3':
            dev = '/dev/mmcblk0'
            kernelA = dev + 'p2'
            rootfsA = dev + 'p3'
            kernelB = dev + 'p4'
            rootfsB = dev + 'p5'
        elif rootdev == '/dev/mmcblk0p5':
            dev = '/dev/mmcblk0'
            kernelA = dev + 'p4'
            rootfsA = dev + 'p5'
            kernelB = dev + 'p2'
            rootfsB = dev + 'p3'
        else:
            raise error.TestError('Unexpected root device %s' % rootdev)
        return dev, kernelA, rootfsA, kernelB, rootfsB


    def _corrupt_rootfs(self, host):
        """Corrupt the root file system
        """

        self.client = host
        self.client_test = 'platform_CorruptRootfs'

        dev, kernelA, rootfsA, kernelB, rootfsB = self._get_partition_layout()
        bootcache_offset = self._get_bootcache_offset()

        # Copy kernel and rootfs paritions from A to B
        logging.info('CorruptRootfs: copy partitions A to B')
        self.client.run('dd if=%s of=%s bs=64K' % (kernelA, kernelB))
        self.client.run('dd if=%s of=%s bs=64K' % (rootfsA, rootfsB))

        # Set attribrutes on kernel A and B
        logging.info('CorruptRootfs: set attributes on kernal A and B')
        self.client.run('cgpt add -i 2 -T 5 -P 9 -S 1 %s' % dev)
        self.client.run('cgpt add -i 4 -T 5 -P 9 -S 1 %s' % dev)

        # Corrupt rootfs A and bootcache
        logging.info('CorruptRootfs: corrupt rootfs A ' + rootfsA)
        self.client.run('dd if=/dev/urandom of=%s count=16' % rootfsA)
        if bootcache_offset != '0':
            self.client.run('dd if=/dev/zero of=%s seek=%s count=4096' %
                (rootfsA, bootcache_offset))

        logging.info('CorruptRootfs: reboot ' + self.client.hostname)
        try:
            self.client.reboot()
        except error.AutoservRebootError as e:
            raise error.TestFail('%s.\nTest failed with error %s' % (
                    traceback.format_exc(), str(e)))

        # Find what partition we are now running on
        result = self.client.run('rootdev -s')
        logging.info('Root partition %s', result.stdout)


    def run_once(self, host=None):
        """
        run_once actually runs the test twice. The second run "undoes"
        what was done in the first run.

        @param host - the host machine running the test
        """

        self._corrupt_rootfs(host)
        self._corrupt_rootfs(host)
