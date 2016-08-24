# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.common_lib import error
from autotest_lib.server import test


class platform_Vpd(test.test):
    """Test that vpd's cache gets generated during the boot if missing.

    Clean the vpd cache file and check that the files are correctly generated
    at the next reboot.
    """
    version = 1

    _VPD_FILES = [
        ('/mnt/stateful_partition/unencrypted/cache/vpd/filtered.txt',
            'root', 'root', '644'),
        ('/mnt/stateful_partition/unencrypted/cache/vpd/echo/vpd_echo.txt',
            'root', 'chronos', '640'),
        ('/mnt/stateful_partition/unencrypted/cache/vpd/full-v2.txt',
            'root', 'root', '600')
    ]

    _VPD_LINKS = [
        '/var/log/vpd_2.0.txt',
        '/var/cache/vpd/full-v2.txt',
        '/var/cache/echo/vpd_echo.txt'
    ]

    # files used by older versions on vpd and that should not be here anymore
    _VPD_OLD_FILES = [
        '/var/cache/vpd/full.cache',
        '/var/cache/offers/vpd_echo.txt',
        '/var/cache/vpd/full-v2.cache'
    ]

    def get_stat(self, host, path):
        """Return user, group and permissions of file on host.

        @param host: the host machine to test
        @param path: path to the file that we are testing

        @return None if the file does not exist
                (user, group, permissions) if it does.
        """
        if not self.file_exists(host, path):
            return None

        user = host.run('stat -c %U ' + path).stdout.strip()
        group = host.run('stat -c %G ' + path).stdout.strip()
        mode = host.run('stat -c %a ' + path).stdout.strip()

        return (user, group, mode)

    def file_exists(self, host, path):
        """Check if the path exists.

        @param host: the host machine
        @param path: path of the file to check

        @return True if the file exists
        """
        return host.run('[ -f %s ]' % path,
                        ignore_status=True).exit_status == 0

    def is_symlink(self, host, path):
        """Check if a file is a symlink.

        @param host: the host machine
        @param path: path to the file

        @return True is the file is a symlink
        """
        return host.run('[ -h %s ]' % path,
                        ignore_status=True).exit_status == 0

    def run_once(self, host):
        host.run('dump_vpd_log --clean')

        removed_files = [item[0] for item in self._VPD_FILES]
        removed_files += self._VPD_LINKS
        removed_files += self._VPD_OLD_FILES

        for vpdfile in removed_files:
            if self.file_exists(host, vpdfile):
                raise error.TestFail('Vpd file %s was not removed by '
                    'dump_vpd_log --clean' % vpdfile)

        host.reboot()

        # check that the files exist and have the right permissions
        for (path, user, group, perm) in self._VPD_FILES:
            if self.is_symlink(host, path):
                raise error.TestFail('File %s should not be a symlink' % path)

            stats = self.get_stat(host, path)
            if stats is None:
                raise error.TestFail('File %s should be present' % path)

            if user != stats[0]:
                raise error.TestFail('Wrong user (%s instead of %s) for %s' %
                      (stats[0], user, path))

            if group != stats[1]:
                raise error.TestFail('Wrong group (%s instead of %s) for %s' %
                      (stats[1], group, path))

            if perm != stats[2]:
                raise error.TestFail('Wrong permissions (%s instead of %s)'
                    ' for %s' % (stats[2], perm, path))

        # for symlinks, check that they exist and are symlinks
        for path in self._VPD_LINKS:
            if not self.is_symlink(host, path):
                raise error.TestFail('%s should be a symlink' % path)

            if not self.file_exists(host, path):
                raise error.TestFail('Symlink %s does not exist' % path)

        for path in self._VPD_OLD_FILES:
            if self.file_exists(host, path):
                raise error.TestFail('Old vpd file %s installed' % path)
