# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os
import tarfile
import zipfile

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import autotemp, error
from autotest_lib.client.cros.cros_disks import CrosDisksTester
from autotest_lib.client.cros.cros_disks import VirtualFilesystemImage
from autotest_lib.client.cros.cros_disks import DefaultFilesystemTestContent
from collections import deque


class CrosDisksArchiveTester(CrosDisksTester):
    """A tester to verify archive support in CrosDisks.
    """
    def __init__(self, test, archive_types):
        super(CrosDisksArchiveTester, self).__init__(test)
        self._archive_types = archive_types

    def _find_all_files(self, root_dir):
        """Returns all files under a directory and its sub-directories.

           This is a generator that performs a breadth-first-search of
           all files under a specified directory and its sub-directories.

        Args:
            root_dir: The root directory where the search starts from.
        Yields:
            Path of any found file relative to the root directory.
        """
        dirs_to_explore = deque([''])
        while len(dirs_to_explore) > 0:
            current_dir = dirs_to_explore.popleft()
            for path in os.listdir(os.path.join(root_dir, current_dir)):
                expanded_path = os.path.join(root_dir, current_dir, path)
                relative_path = os.path.join(current_dir, path)
                if os.path.isdir(expanded_path):
                    dirs_to_explore.append(relative_path)
                else:
                    yield relative_path

    def _make_tar_archive(self, archive_path, root_dir, compression=None):
        """Archives a specified directory into a tar file.

           The created tar file contains all files and sub-directories
           under the specified root directory, but not the root directory
           itself.

        Args:
            archive_path: Path of the output archive.
            root_dir: The root directory to archive.
            compression: The compression method: None, 'gz', 'bz2'
        """
        mode = 'w:' + compression if compression else 'w'
        # TarFile in Python 2.6 does not work with the 'with' statement.
        archive = tarfile.open(archive_path, mode)
        for path in self._find_all_files(root_dir):
            archive.add(os.path.join(root_dir, path), path)
        archive.close()

    def _make_zip_archive(self, archive_path, root_dir,
                         compression=zipfile.ZIP_DEFLATED):
        """Archives a specified directory into a ZIP file.

           The created ZIP file contains all files and sub-directories
           under the specified root directory, but not the root directory
           itself.

        Args:
            archive_path: Path of the output archive.
            root_dir: The root directory to archive.
            compression: The ZIP compression method.
        """
        # ZipFile in Python 2.6 does not work with the 'with' statement.
        archive = zipfile.ZipFile(archive_path, 'w', compression)
        for path in self._find_all_files(root_dir):
            archive.write(os.path.join(root_dir, path), path)
        archive.close()

    def _make_archive(self, archive_type, archive_path, root_dir):
        """Archives a specified directory into an archive of specified type.

           The created archive file contains all files and sub-directories
           under the specified root directory, but not the root directory
           itself.

        Args:
            archive_type: Type of the output archive.
            archive_path: Path of the output archive.
            root_dir: The root directory to archive.
        """
        if archive_type in ['zip']:
            self._make_zip_archive(archive_path, root_dir)
        elif archive_type in ['tar']:
            self._make_tar_archive(archive_path, root_dir)
        elif archive_type in ['tar.gz', 'tgz']:
            self._make_tar_archive(archive_path, root_dir, 'gz')
        elif archive_type in ['tar.bz2', 'tbz', 'tbz2']:
            self._make_tar_archive(archive_path, root_dir, 'bz2')
        else:
            raise error.TestFail("Unsupported archive type " + archive_type)

    def _test_archive(self, archive_type):
        # Create the archive file content in a temporary directory.
        archive_dir = autotemp.tempdir(unique_id='CrosDisks')
        test_content = DefaultFilesystemTestContent()
        if not test_content.create(archive_dir.name):
            raise error.TestFail("Failed to create archive test content")

        # Create a FAT-formatted virtual filesystem image containing an
        # archive file to help stimulate mounting an archive file on a
        # removable drive.
        with VirtualFilesystemImage(
                block_size=1024,
                block_count=65536,
                filesystem_type='vfat',
                mkfs_options=[ '-F', '32', '-n', 'ARCHIVE' ]) as image:
            image.format()
            image.mount(options=['sync'])
            # Create the archive file on the virtual filesystem image.
            archive_name = 'test.' + archive_type
            archive_path = os.path.join(image.mount_dir, archive_name)
            self._make_archive(archive_type, archive_path, archive_dir.name)
            image.unmount()

            # Mount the virtual filesystem image via CrosDisks.
            device_file = image.loop_device
            self.cros_disks.mount(device_file, '',
                                  [ "ro", "nodev", "noexec", "nosuid" ])
            result = self.cros_disks.expect_mount_completion({
                'status': 0,
                'source_path': device_file
            })

            # Mount the archive file on the mounted filesystem via CrosDisks.
            archive_path = os.path.join(result['mount_path'], archive_name)
            expected_mount_path = os.path.join('/media/archive', archive_name)
            self.cros_disks.mount(archive_path)
            result = self.cros_disks.expect_mount_completion({
                'status': 0,
                'source_path': archive_path,
                'mount_path': expected_mount_path
            })

            # Verify the content of the mounted archive file.
            if not test_content.verify(expected_mount_path):
                raise error.TestFail("Failed to verify filesystem test content")

            self.cros_disks.unmount(expected_mount_path, ['force'])
            self.cros_disks.unmount(device_file, ['force'])

    def test_archives(self):
        for archive_type in self._archive_types:
            self._test_archive(archive_type)

    def get_tests(self):
        return [self.test_archives]


class platform_CrosDisksArchive(test.test):
    version = 1

    def run_once(self, *args, **kwargs):
        tester = CrosDisksArchiveTester(self, kwargs['archive_types'])
        tester.run(*args, **kwargs)
