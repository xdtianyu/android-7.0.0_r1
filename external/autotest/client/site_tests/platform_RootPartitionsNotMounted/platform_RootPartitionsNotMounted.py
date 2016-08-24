# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os

from autotest_lib.client.bin import site_utils, test
from autotest_lib.client.common_lib import error, utils

class platform_RootPartitionsNotMounted(test.test):
    version = 1

    _CGPT_PATH = '/usr/bin/cgpt'
    _ROOTDEV_PATH = '/usr/bin/rootdev'
    _UPDATE_ENGINE_PATH = '/usr/sbin/update_engine'

    def get_root_partitions(self, device):
        """Gets a list of root partitions of a device.

        Gets a list of root partitions of a device by calling
        `cgpt find -t rootfs <device>`.

        Args:
            device: The device, specified by its device file, to examine.

        Returns:
            A list of root partitions, specified by their device file,
            (e.g. /dev/sda1) of the given device.
        """
        cgpt_command = '%s find -t rootfs %s' % (self._CGPT_PATH, device)
        return utils.run(cgpt_command).stdout.strip('\n').split('\n')

    def get_mounted_devices(self, mounts_file):
        """Gets a set of mounted devices from a given mounts file.

        Gets a set of device files that are currently mounted. This method
        parses a given mounts file (e.g. /proc/<pid>/mounts) and extracts the
        entries with a source path under /dev/.

        Returns:
            A set of device file names (e.g. /dev/sda1)
        """
        mounted_devices = set()
        try:
            entries = open(mounts_file).readlines()
        except:
            entries = []
        for entry in entries:
            node = entry.split(' ')[0]
            if node.startswith('/dev/'):
                mounted_devices.add(node)
        return mounted_devices

    def get_process_executable(self, pid):
        """Gets the executable path of a given process ID.

        Args:
            pid: Target process ID.

        Returns:
            The executable path of the given process ID or None on error.
        """
        try:
            return os.readlink('/proc/%s/exe' % pid)
        except:
            return ""

    def get_process_list(self, excluded_executables=[]):
        """Gets a list of process IDs of active processes.

        Gets a list of process IDs of active processes by looking into /proc
        and filters out those processes with a executable path that is
        excluded.

        Args:
            excluded_executables: A list of executable paths to exclude.

        Returns:
            A list of process IDs of active processes.
        """
        processes = []
        for path in os.listdir('/proc'):
            if not path.isdigit(): continue
            process_exe = self.get_process_executable(path)
            if process_exe and process_exe not in excluded_executables:
                processes.append(path)
        return processes

    def run_once(self):
        if os.geteuid() != 0:
            raise error.TestNAError('This test needs to be run under root')

        for path in [self._CGPT_PATH, self._ROOTDEV_PATH]:
            if not os.path.isfile(path):
                raise error.TestNAError('%s not found' % path)

        root_device = site_utils.get_root_device()
        if not root_device:
            raise error.TestNAError('Could not find the root device')
        logging.debug('Root device: %s' % root_device)

        root_partitions = self.get_root_partitions(root_device)
        if not root_partitions:
            raise error.TestNAError('Could not find any root partition')
        logging.debug('Root partitions: %s' % ', '.join(root_partitions))

        processes = self.get_process_list([self._UPDATE_ENGINE_PATH])
        if not processes:
            raise error.TestNAError('Could not find any process')
        logging.debug('Active processes: %s' % ', '.join(processes))

        for process in processes:
            process_exe = self.get_process_executable(process)
            mounts_file = '/proc/%s/mounts' % process
            mounted_devices = self.get_mounted_devices(mounts_file)
            for partition in root_partitions:
                if partition in mounted_devices:
                    raise error.TestFail(
                            'Root partition "%s" is mounted by process %s (%s)'
                            % (partition, process, process_exe))
