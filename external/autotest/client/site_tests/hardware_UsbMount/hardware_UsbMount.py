# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
# Author: Cosimo Alfarano <cosimo.alfarano@collabora.co.uk>

import logging, os

from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import rtc, sys_power
from cros import storage as storage_mod


class hardware_UsbMount(storage_mod.StorageTester):
    version = 1
    SECS_TO_SUSPEND = 10
    _tmpfile = None


    def cleanup(self):
        # if the test fails with the device unmounted and before re-mounting,
        # the test will be unable to properly cleanup, since we have no way to
        # remove a tmp file from an unmounted device.
        # For instance, this can happen if suspend won't work (e.g. it will
        # reboot instead).
        if self._tmpfile and os.path.isfile(self._tmpfile):
            logging.debug('cleanup(): removing %s', self._tmpfile)
            os.remove(self._tmpfile)

        self.scanner.unmount_all()

        super(hardware_UsbMount, self).cleanup()


    def run_once(self, mount_cycles=10, filter_dict={'bus':'usb'}):
        """
        @param mount_cycles: how many times to mount/unount Default: 10.
        @param filter_dict: storage dictionary filter.
               Default: match any device connected on the USB bus.
        """
        # wait_for_device() returns (device_dictionary,
        # time_spent_looking_for_it), and only the dictionary is relevant for
        # this test
        storage = self.wait_for_device(filter_dict, cycles=1,
                                       mount_volume=True)[0]

        if not os.path.ismount(storage['mountpoint']):
            raise error.TestFail('filesystem %s mount failed' % filter_dict)

        storage_filter = {'fs_uuid': storage['fs_uuid']}
        # We cannot use autotemp.tempfile since we should close the descriptors
        # everytime the storage device is un-mounted.
        self._tmpfile = os.path.join(storage['mountpoint'],
                                     'tempfile_usb_mount.tmp')

        while mount_cycles:
            mount_cycles -= 1
            # Create a 1MiB random file and checksum it.
            storage_mod.create_file(self._tmpfile, 1*1024*1024)
            chksum = storage_mod.checksum_file(self._tmpfile)

            logging.debug('storage to umount %s', storage)

            # Umount the volume.
            self.scanner.umount_volume(storage_dict=storage)
            storage = self.wait_for_device(storage_filter,
                                           mount_volume=False)[0]
            if os.path.ismount(storage['mountpoint']):
                raise error.TestFail('filesystem %s unmount failed ' %
                                     storage_filter)

            # Mount the volume back.
            self.scanner.mount_volume(storage_dict=storage)
            storage =  self.wait_for_device(storage_filter,
                                            mount_volume=False)[0]
            if not os.path.ismount(storage['mountpoint']):
                raise error.TestFail('filesystem %s mount failed' %
                                     storage_filter)

            # Check that the created file exists and has the same content.
            if not os.path.isfile(self._tmpfile):
                raise error.TestFail('%s: file not present after remounting' %
                                     self._tmpfile)

            if chksum != storage_mod.checksum_file(self._tmpfile):
                raise error.TestFail('%s: file content changed after '
                                     'remounting' % self._tmpfile)

        # Mount it, suspend and verify that after suspend-to-ram the
        # device is still mounted
        self.scanner.mount_volume(storage_dict=storage)
        storage = self.wait_for_device(storage_filter, mount_volume=False)[0]
        if not os.path.ismount(storage['mountpoint']):
            raise error.TestFail('filesystem %s mount failed ' % storage)

        sys_power.do_suspend(self.SECS_TO_SUSPEND)

        # mount_volume=False because we don't want the method to mount if
        # unmonted: we need to check its actual status right after suspend
        storage = self.wait_for_device(storage_filter, mount_volume=False)[0]

        if not os.path.ismount(storage['mountpoint']):
            raise error.TestFail('filesystem %s not mounted after suspend' %
                                 storage_filter)

        if not os.path.isfile(self._tmpfile):
            raise error.TestFail('%s: file not present anymore after '
                                 'remounting' % self._tmpfile)

        if chksum != storage_mod.checksum_file(self._tmpfile):
            raise error.TestFail('%s: file content changed after remounting' %
                                 self._tmpfile)
