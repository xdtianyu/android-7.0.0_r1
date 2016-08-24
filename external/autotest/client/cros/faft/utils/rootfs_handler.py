# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""A module containing rootfs handler class."""

import os
import re

TMP_FILE_NAME = 'kernel_dump'

_KERNEL_MAP = {'A': '2', 'B': '4'}
_ROOTFS_MAP = {'A': '3', 'B': '5'}
_DM_DEVICE = 'verifyroot'
_DM_DEV_PATH = os.path.join('/dev/mapper', _DM_DEVICE)


class RootfsHandler(object):
    """An object to provide ChromeOS root FS related actions.

    It provides functions to verify the integrity of the root FS.
    """

    def __init__(self):
        self.os_if = None
        self.root_dev = None
        self.kernel_dump_file = None

    def verify_rootfs(self, section):
        """Verifies the integrity of the root FS.

        @param section: The rootfs to verify. May be A or B.
        """
        kernel_path = self.os_if.join_part(self.root_dev,
                _KERNEL_MAP[section.upper()])
        rootfs_path = self.os_if.join_part(self.root_dev,
                _ROOTFS_MAP[section.upper()])
        # vbutil_kernel won't operate on a device, only a file.
        self.os_if.run_shell_command(
                'dd if=%s of=%s' % (kernel_path, self.kernel_dump_file))
        vbutil_kernel = self.os_if.run_shell_command_get_output(
                'vbutil_kernel --verify %s --verbose' % self.kernel_dump_file)
        DM_REGEXP = re.compile(r'dm="(?:1 )?vroot none ro(?: 1)?,(0 (\d+) .+)"')
        match = DM_REGEXP.search('\n'.join(vbutil_kernel))
        if not match:
            return False

        table = match.group(1)
        partition_size = int(match.group(2)) * 512

        assert 'PARTUUID=%U/PARTNROFF=1' in table
        table = table.replace('PARTUUID=%U/PARTNROFF=1', rootfs_path)
        # Cause I/O error on invalid bytes
        table += ' error_behavior=eio'

        self._remove_mapper()
        assert not self.os_if.path_exists(_DM_DEV_PATH)
        self.os_if.run_shell_command(
                "dmsetup create -r %s --table '%s'" % (_DM_DEVICE, table))
        assert self.os_if.path_exists(_DM_DEV_PATH)
        try:
            count = self.os_if.get_file_size(_DM_DEV_PATH)
            return count == partition_size
        except:
            return False
        finally:
            self._remove_mapper()

    def _remove_mapper(self):
        """Removes the dm device mapper used by this class."""
        if self.os_if.path_exists(_DM_DEV_PATH):
            self.os_if.run_shell_command_get_output(
                    'dmsetup remove %s' % _DM_DEVICE)

    def init(self, os_if):
        """Initialize the rootfs handler object.

        @param os_if: OS interface object reference.
        """
        self.os_if = os_if
        self.root_dev = os_if.get_root_dev()
        self.kernel_dump_file = os_if.state_dir_file(TMP_FILE_NAME)
