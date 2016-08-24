#!/usr/bin/python
#
# Copyright (c) 2010 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

__author__ = 'kdlucas@chromium.org (Kelly Lucas)'

import logging
import os
import re
import shutil
import stat

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error

class platform_FilePerms(test.test):
    """
    Test file permissions.
    """
    version = 2
    mount_path = '/bin/mount'
    standard_rw_options = ['rw', 'nosuid', 'nodev', 'noexec', 'relatime']
    # When adding an expectation that isn't simply "standard_rw_options,"
    # please leave either an explanation for why that mount is special,
    # or a bug number tracking work to harden that mount point, in a comment.
    expected_mount_options = {
        '/dev': {
            'type': 'devtmpfs',
            'options': ['rw', 'nosuid', 'noexec', 'relatime', 'mode=755']},
        '/dev/pstore': {
            'type': 'pstore',
            'options': standard_rw_options},
        '/dev/pts': { # Special case, we want to track gid/mode too.
            'type': 'devpts',
            'options': ['rw', 'nosuid', 'noexec', 'relatime', 'gid=5',
                        'mode=620']},
        '/dev/shm': {'type': 'tmpfs', 'options': standard_rw_options},
        '/home': {'type': 'ext4', 'options': standard_rw_options},
        '/home/chronos': {'type': 'ext4', 'options': standard_rw_options},
        '/media': {'type': 'tmpfs', 'options': standard_rw_options},
        '/mnt/stateful_partition': {
            'type': 'ext4',
            'options': standard_rw_options},
        '/mnt/stateful_partition/encrypted': {
            'type': 'ext4',
            'options': standard_rw_options},
        '/proc': {'type': 'proc', 'options': standard_rw_options},
        '/run': { # Special case, we want to track mode too.
            'type': 'tmpfs',
            'options': ['rw', 'nosuid', 'nodev', 'noexec', 'relatime',
                        'mode=755']},
        # Special case, we want to track group/mode too.
        # gid 236 == debugfs-access
        '/run/debugfs_gpu': {
            'type': 'debugfs',
            'options': ['rw', 'nosuid', 'nodev', 'noexec', 'relatime',
                        'gid=236', 'mode=750']},
        '/run/lock': {'type': 'tmpfs', 'options': standard_rw_options},
        '/sys': {'type': 'sysfs', 'options': standard_rw_options},
        '/sys/fs/cgroup': {
            'type': 'tmpfs',
            'options': standard_rw_options + ['mode=755']},
        '/sys/fs/cgroup/cpu': {
            'type': 'cgroup',
            'options': standard_rw_options},
        '/sys/fs/cgroup/cpuacct': {
            'type': 'cgroup',
            'options': standard_rw_options},
        '/sys/fs/cgroup/devices': {
            'type': 'cgroup',
            'options': standard_rw_options},
        '/sys/fs/cgroup/freezer': {
            'type': 'cgroup',
            'options': standard_rw_options},
        '/sys/fs/fuse/connections': {
            'type': 'fusectl',
            'options': standard_rw_options},
        '/sys/kernel/debug': {
            'type': 'debugfs',
            'options': standard_rw_options},
        '/tmp': {'type': 'tmpfs', 'options': standard_rw_options},
        '/var': {'type': 'ext4', 'options': standard_rw_options},
        '/usr/share/oem': {
            'type': 'ext4',
            'options': ['ro', 'nosuid', 'nodev', 'noexec', 'relatime']},
    }
    testmode_modded_fses = set(['/home', '/tmp', '/usr/local'])


    def checkid(self, fs, userid):
        """
        Check that the uid and gid for |fs| match |userid|.

        @param fs: string, directory or file path.
        @param userid: userid to check for.
        Returns:
            int, the number errors (non-matches) detected.
        """
        errors = 0

        if not os.access(fs, os.F_OK):
            # The path does not exist, so exit early.
            return errors

        uid = os.stat(fs)[stat.ST_UID]
        gid = os.stat(fs)[stat.ST_GID]

        if userid != uid:
            logging.error('Wrong uid in filesystem "%s"', fs)
            errors += 1
        if userid != gid:
            logging.error('Wrong gid in filesystem "%s"', fs)
            errors += 1

        return errors


    def get_perm(self, fs):
        """
        Check the file permissions of a filesystem.

        @param fs: string, mount point for filesystem to check.
        Returns:
            int, equivalent to unix permissions.
        """
        MASK = 0777

        if not os.access(fs, os.F_OK):
            # The path does not exist, so exit early.
            return None

        fstat = os.stat(fs)
        mode = fstat[stat.ST_MODE]

        fperm = oct(mode & MASK)
        return fperm


    def read_mtab(self, mtab_path='/etc/mtab'):
        """
        Helper function to read the mtab file into a dict

        @param mtab_path: path to '/etc/mtab'
        Returns:
          dict, mount points as keys, and another dict with
          options list, device and type as values.
        """
        file_handle = open(mtab_path, 'r')
        lines = file_handle.readlines()
        file_handle.close()

        # Save mtab to the results dir to diagnose failures.
        shutil.copyfile(mtab_path,
                        os.path.join(self.resultsdir,
                                     os.path.basename(mtab_path)))

        comment_re = re.compile("#.*$")
        mounts = {}
        for line in lines:
            # remove any comments first
            line = comment_re.sub("", line)
            fields = line.split()
            # ignore malformed lines
            if len(fields) < 4:
                continue
            # Don't include rootfs in the list, because it maps to the
            # same location as /dev/root: '/' (and we don't care about
            # its options at the moment).
            if fields[0] == 'rootfs':
                continue
            mounts[fields[1]] = {'device': fields[0],
                                 'type': fields[2],
                                 'options': fields[3].split(',')}
        return mounts


    def try_write(self, fs):
        """
        Try to write a file in the given filesystem.

        @param fs: string, file system to use.
        Returns:
            int, number of errors encountered:
            0 = write successful,
            >0 = write not successful.
        """

        TEXT = 'This is filler text for a test file.\n'

        tempfile = os.path.join(fs, 'test')
        try:
            fh = open(tempfile, 'w')
            fh.write(TEXT)
            fh.close()
        except OSError: # This error will occur with read only filesystem.
            return 1
        except IOError, e:
            return 1

        if os.path.exists(tempfile):
            os.remove(tempfile)

        return 0


    def check_mounted_read_only(self, filesystem):
        """
        Check the permissions of a filesystem according to /etc/mtab.

        @param filesystem: string, file system device to check.
        Returns:
            1 if rw, 0 if ro
        """

        errors = 0
        mtab = self.read_mtab()
        if not (filesystem in mtab.keys()):
            logging.error('Could not find filesystem "%s" in mtab', filesystem)
            errors += 1
            return errors # no point in continuing this test.
        if not ('ro' in mtab[filesystem]['options']):
            logging.error('Filesystem "%s" is not mounted read-only',
                          filesystem)
            errors += 1
        return errors


    def check_mount_options(self):
        """
        Check the permissions of all non-rootfs filesystems to make
        sure they have the right mount options. In order to do this,
        both the live system state, and a log-snapshot of what the system
        looked like prior to dev-mode/test-mode modifications were applied,
        are validated.

        Note that since this test is not a UITest, and takes place
        while the system waits at a login screen, mount options are
        not checked for a mounted cryptohome or guestfs. Consult the
        security_ProfilePermissions test for those checks.

        Args:
            (none)
        Returns:
            int, the number of errors identified in mount options.
        """
        errors = 0
        # Perform mount-option checks of both mount options as
        # captured during boot, and, the live system state.  After the
        # first pass (where we process mount_options.log), grow the
        # list of ignored filesystems to include all those we know are
        # tweaked by devmode/mod-for-test mode. This properly sets
        # expectations for the second pass.
        mtabs = ['/var/log/mount_options.log', '/etc/mtab']
        ignored_fses = set(['/'])
        ignored_types = set(['ecryptfs'])
        for mtab_path in mtabs:
            mtab = self.read_mtab(mtab_path=mtab_path)
            for fs in mtab.keys():
                if fs in ignored_fses:
                    continue

                fs_type = mtab[fs]['type']
                if fs_type in ignored_types:
                    logging.warning('Ignoring filesystem "%s" with type "%s"',
                                 fs, fs_type)
                    continue
                if not fs in self.expected_mount_options:
                    logging.error('No expectations entry for "%s"', fs)
                    errors += 1
                    continue

                if fs_type != self.expected_mount_options[fs]['type']:
                    logging.error(
                            '[%s] "%s" has type "%s", expected type "%s"',
                            mtab_path, fs, fs_type,
                            self.expected_mount_options[fs]['type'])
                    errors += 1

                # For options, require the specified options to be present.
                # Do not consider it an error if extra options are present.
                # (This makes it easy to deal with options we don't wish
                # to track closely, like devtmpfs's nr_inodes= for example.)
                seen = set(mtab[fs]['options'])
                expected = set(self.expected_mount_options[fs]['options'])
                missing = expected - seen
                if (missing):
                    logging.error('[%s] "%s" is missing options "%s"',
                                  mtab_path, fs, missing)
                    errors += 1

            ignored_fses.update(self.testmode_modded_fses)

        return errors


    def run_once(self):
        """
        Main testing routine for platform_FilePerms.
        """
        errors = 0

        # Root owned directories with expected permissions.
        root_dirs = {'/': ['0755'],
                     '/bin': ['0755'],
                     '/boot': ['0755'],
                     '/dev': ['0755'],
                     '/etc': ['0755'],
                     '/home': ['0755'],
                     '/lib': ['0755'],
                     '/media': ['0777'],
                     '/mnt': ['0755'],
                     '/mnt/stateful_partition': ['0755'],
                     '/opt': ['0755'],
                     '/proc': ['0555'],
                     '/run': ['0755'],
                     '/sbin': ['0755'],
                     '/sys': ['0555', '0755'],
                     '/tmp': ['0777'],
                     '/usr': ['0755'],
                     '/usr/bin': ['0755'],
                     '/usr/lib': ['0755'],
                     '/usr/local': ['0755'],
                     '/usr/sbin': ['0755'],
                     '/usr/share': ['0755'],
                     '/var': ['0755'],
                     '/var/cache': ['0755']}

        # Read-only directories
        ro_dirs = ['/', '/bin', '/boot', '/etc', '/lib', '/mnt',
                   '/opt', '/sbin', '/usr', '/usr/bin', '/usr/lib',
                   '/usr/sbin', '/usr/share']

        # Root directories writable by root
        root_rw_dirs = ['/run', '/var', '/var/lib', '/var/cache', '/var/log',
                        '/usr/local']

        # Ensure you cannot write files in read only directories.
        for dir in ro_dirs:
            if self.try_write(dir) == 0:
                logging.error('Root can write to read-only dir "%s"', dir)
                errors += 1

        # Ensure the uid and gid are correct for root owned directories.
        for dir in root_dirs:
            if self.checkid(dir, 0) > 0:
                errors += 1

        # Ensure root can write into root dirs with rw access.
        for dir in root_rw_dirs:
            if self.try_write(dir) > 0:
                errors += 1

        # Check permissions on root owned directories.
        for dir in root_dirs:
            fperms = self.get_perm(dir)
            if fperms is not None and fperms not in root_dirs[dir]:
                logging.error('"%s" has "%s" permissions', dir, fperms)
                errors += 1

        errors += self.check_mounted_read_only('/')

        # Check mount options on mount points.
        errors += self.check_mount_options()

        # If errors is not zero, there were errors.
        if errors > 0:
            raise error.TestFail('Found %d permission errors' % errors)
