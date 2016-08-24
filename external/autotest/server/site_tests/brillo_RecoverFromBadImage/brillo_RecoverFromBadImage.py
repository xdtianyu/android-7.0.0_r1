# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os
import re

import common
from autotest_lib.client.common_lib import error
from autotest_lib.server import test


# The /dev directory mapping partition names to block devices.
_BLK_DEV_BY_NAME_DIR = '/dev/block/by-name'
# By default, we kill and recover the active system partition.
_DEFAULT_PART_NAME = 'system_X'


class brillo_RecoverFromBadImage(test.test):
    """Ensures that a Brillo device can recover from a bad image."""
    version = 1


    def resolve_slot(self, host, partition):
        """Resolves a partition slot (if any).

        @param host: A host object representing the DUT.
        @param partition: The name of the partition we are using. If it ends
                          with '_X' then we attempt to substitute it with some
                          non-active slot.

        @return A pair consisting of a fully resolved partition name and slot
                index; the latter is None if the partition is not slotted.

        @raise TestError: If a target slot could not be resolved.
        """
        # Check if the partition is slotted.
        if not re.match('.+_[a-zX]$', partition):
            return partition, None

        try:
            current_slot = int(
                    host.run_output('bootctl get-current-slot').strip())
            if partition[-1] == 'X':
                # Find a non-active target slot we could use.
                num_slots = int(
                        host.run_output('bootctl get-number-slots').strip())
                if num_slots < 2:
                    raise error.TestError(
                            'Device has no non-active slot that we can use')
                target_slot = 0 if current_slot else 1
                partition = partition[:-1] + chr(ord('a') + target_slot)
                logging.info(
                        'Current slot is %d, partition resolved to %s '
                        '(slot %d)', current_slot, partition, target_slot)
            else:
                # Make sure the partition slot is different from the active one.
                target_slot = ord(partition[-1]) - ord('a')
                if target_slot == current_slot:
                    target_slot = None
                    logging.warning(
                            'Partition %s is associated with the current boot '
                            'slot (%d), wiping it might fail if it is mounted',
                            partition, current_slot)
        except error.AutoservError:
            raise error.TestError('Error resolving device slots')

        return partition, target_slot


    def find_partition_device(self, host, partition):
        """Returns the block device of the partition.

        @param host: A host object representing the DUT.
        @param partition: The name of the partition we are using.

        @return Path to the device containing the partition.

        @raise TestError: If the partition name could not be mapped to a device.
        """
        try:
            cmd = 'find %s -type l' % os.path.join(_BLK_DEV_BY_NAME_DIR, '')
            for device in host.run_output(cmd).splitlines():
                if os.path.basename(device) == partition:
                    logging.info('Mapped partition %s to device %s',
                                 partition, device)
                    return device
        except error.AutoservError:
            raise error.TestError(
                    'Error finding device for partition %s' % partition)
        raise error.TestError(
                'No device found for partition %s' % partition)


    def get_device_block_info(self, host, device):
        """Returns the block size and count for a device.

        @param host: A host object representing the DUT.
        @param device: Path to a block device.

        @return A pair consisting of the block size (in bytes) and the total
                number of blocks on the device.

        @raise TestError: If we failed to get the block info for the device.
        """
        try:
            block_size = int(
                    host.run_output('blockdev --getbsz %s' % device).strip())
            device_size = int(
                    host.run_output('blockdev --getsize64 %s' % device).strip())
        except error.AutoservError:
            raise error.TestError(
                    'Failed to get block info for device %s' % device)
        return block_size, device_size / block_size


    def run_once(self, host=None, image_file=None, partition=_DEFAULT_PART_NAME,
                 device=None):
        """Runs the test.

        @param host: A host object representing the DUT.
        @param image_file: Image file to flash to the partition.
        @param partition: Name of the partition to wipe/recover.
        @param device: Path to the partition block device.

        @raise TestError: Something went wrong while trying to execute the test.
        @raise TestFail: The test failed.
        """
        # Check that the image file exists.
        if image_file is None:
            raise error.TestError('No image file provided')
        if not os.path.isfile(image_file):
            raise error.TestError('Image file %s not found' % image_file)

        try:
            # Resolve partition name and slot.
            partition, target_slot = self.resolve_slot(host, partition)

            # Figure out the partition device.
            if device is None:
                device = self.find_partition_device(host, partition)

            # Find the block size and count for the device.
            block_size, num_blocks = self.get_device_block_info(host, device)

            # Wipe the partition.
            logging.info('Wiping partition %s (%s)', partition, device)
            cmd = ('dd if=/dev/zero of=%s bs=%d count=%d' %
                   (device, block_size, num_blocks))
            run_err = 'Failed to wipe partition using %s' % cmd
            host.run(cmd)

            # Switch to the target slot, if required.
            if target_slot is not None:
                run_err = 'Error setting the active boot slot'
                host.run('bootctl set-active-boot-slot %d' % target_slot)

            # Re-flash the partition with fastboot.
            run_err = 'Failed to reboot the device into fastboot'
            host.ensure_bootloader_mode()
            run_err = 'Failed to flash image to partition %s' % partition
            host.fastboot_run('flash', args=(partition, image_file))

            # Reboot the device.
            run_err = 'Failed to reboot the device after flashing image'
            host.ensure_adb_mode()

            # Make sure we've booted from the alternate slot, if required.
            if target_slot is not None:
                run_err = 'Error checking the current boot slot'
                current_slot = int(
                        host.run_output('bootctl get-current-slot').strip())
                if current_slot != target_slot:
                    logging.error('Rebooted from slot %d instead of %d',
                                  current_slot, target_slot)
                    raise error.TestError(
                            'Device did not reboot from the expected slot')
        except error.AutoservError:
            raise error.TestFail(run_err)
