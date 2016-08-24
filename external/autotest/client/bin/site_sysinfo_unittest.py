#!/usr/bin/python

"""Tests for site_sysinfo."""

__author__ = 'dshi@google.com (Dan Shi)'

import common
import os
import random
import unittest
from autotest_lib.client.bin import site_sysinfo
from autotest_lib.client.common_lib import autotemp


class diffable_logdir_test(unittest.TestCase):
    """Tests for methods in class diffable_logdir."""


    def setUp(self):
        """Initialize a temp direcotry with test files."""
        self.tempdir = autotemp.tempdir(unique_id='diffable_logdir')
        self.src_dir = os.path.join(self.tempdir.name, 'src')
        self.dest_dir = os.path.join(self.tempdir.name, 'dest')

        self.existing_files = ['existing_file_'+str(i) for i in range(3)]
        self.existing_files_folder = ['', 'sub', 'sub/sub2']
        self.existing_files_path = [os.path.join(self.src_dir, folder, f)
                                    for f,folder in zip(self.existing_files,
                                                self.existing_files_folder)]
        self.new_files = ['new_file_'+str(i) for i in range(2)]
        self.new_files_folder = ['sub', 'sub/sub3']
        self.new_files_path = [os.path.join(self.src_dir, folder, f)
                                    for f,folder in zip(self.new_files,
                                                self.new_files_folder)]

        # Create some file with random data in source directory.
        for p in self.existing_files_path:
            self.append_text_to_file(str(random.random()), p)


    def tearDown(self):
        """Clearn up."""
        self.tempdir.clean()


    def append_text_to_file(self, text, file_path):
        """Append text to the end of a file, create the file if not existed.

        @param text: text to be appended to a file.
        @param file_path: path to the file.

        """
        dir_name = os.path.dirname(file_path)
        if not os.path.exists(dir_name):
            os.makedirs(dir_name)
        with open(file_path, 'a') as f:
            f.write(text)


    def test_diffable_logdir_success(self):
        """Test the diff function to save new data from a directory."""
        info = site_sysinfo.diffable_logdir(self.src_dir,
                                            keep_file_hierarchy=False,
                                            append_diff_in_name=False)
        # Run the first time to collect file status.
        info.run(log_dir=None, collect_init_status=True)

        # Add new files to the test directory.
        for file_name, file_path in zip(self.new_files,
                                         self.new_files_path):
            self.append_text_to_file(file_name, file_path)

        # Temp file for existing_file_2, used to hold on the inode. If the
        # file is deleted and recreated, its inode might not change.
        existing_file_2 = self.existing_files_path[2]
        existing_file_2_tmp =  existing_file_2 + '_tmp'
        os.rename(existing_file_2, existing_file_2_tmp)

        # Append data to existing file.
        for file_name, file_path in zip(self.existing_files,
                                         self.existing_files_path):
            self.append_text_to_file(file_name, file_path)

        # Remove the tmp file.
        os.remove(existing_file_2_tmp)

        # Run the second time to do diff.
        info.run(self.dest_dir, collect_init_status=False)

        # Validate files in dest_dir.
        for file_name, file_path in zip(self.existing_files+self.new_files,
                                self.existing_files_path+self.new_files_path):
            file_path = file_path.replace('src', 'dest')
            with open(file_path, 'r') as f:
                self.assertEqual(file_name, f.read())


if __name__ == '__main__':
    unittest.main()
