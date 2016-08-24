#!/usr/bin/python
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Clean Staged Images.

This script is responsible for removing older builds from the Chrome OS
devserver. It walks through the files in the images folder, check each found
staged.timestamp and do following.
1. Check if the build target is in the list of targets that need to keep the
   latest build. Skip processing the directory if that's True.
2. Check if the modified time of the timestamp file is older than a given cutoff
   time, e.g., 24 hours before the current time.
3. If that's True, delete the folder containing staged.timestamp.
4. Check if the parent folder of the deleted foler is empty. If that's True,
   delete the parent folder as well. Do so recursively, until it hits the top
   folder, e.g., |~/images|.
"""

import logging
import optparse
import os
import re
import sys
import shutil
import time

# This filename must be kept in sync with devserver's downloader.py
_TIMESTAMP_FILENAME = 'staged.timestamp'
_HOURS_TO_SECONDS = 60 * 60
_EXEMPTED_DIRECTORIES = []

def get_all_timestamp_dirs(root):
    """Get all directories that has timestamp file.

    @param root: The top folder to look for timestamp file.
    @return: An iterator of directories that have timestamp file in it.
    """
    for dir_path, dir_names, file_names in os.walk(root):
        if os.path.basename(dir_path) in _EXEMPTED_DIRECTORIES:
            logging.debug('Skipping %s', dir_path)
            dir_names[:] = []
        elif _TIMESTAMP_FILENAME in file_names:
            dir_names[:] = []
            yield dir_path


def file_is_too_old(build_path, max_age_hours):
    """Test to see if the build at |build_path| is older than |max_age_hours|.

    @param build_path: The path to the build (ie. 'build_dir/R21-2035.0.0')
    @param max_age_hours: The maximum allowed age of a build in hours.
    @return: True if the build is older than |max_age_hours|, False otherwise.
    """
    cutoff = time.time() - max_age_hours * _HOURS_TO_SECONDS
    timestamp_path = os.path.join(build_path, _TIMESTAMP_FILENAME)
    if os.path.exists(timestamp_path):
        age = os.stat(timestamp_path).st_mtime
        if age < cutoff:
            return True
    return False


def try_delete_parent_dir(path, root):
    """Try to delete parent directory if it's empty.

    Recursively attempt to delete parent directory of given path. Only stop if:
    1. parent directory is the root directory used to stage images.
    2. The base name of given path is a valid build path, e.g., R31-4532.0.0 or
       4530.0.0 (for builds staged in *-channel/[platform]/).
    3. The parent directory is not empty.

    @param path: Start path that attempt to delete whose parent directory.
    @param root: root directory that devserver used to stage images, e.g.,
                 |/usr/local/google/home/dshi/images|, must be an absolute path.
    """
    pattern = '(\d+\.\d+\.\d+)'
    match = re.search(pattern, os.path.basename(path))
    if match:
        return

    parent_dir = os.path.abspath(os.path.join(path, os.pardir))
    if parent_dir == root:
        return

    try:
        os.rmdir(parent_dir)
        try_delete_parent_dir(parent_dir, root)
    except OSError:
        pass


def prune_builds(builds_dir, keep_duration, keep_paladin_duration):
    """Prune the build dirs and also delete old labels.

    @param builds_dir: The builds dir where all builds are staged.
      on the chromeos-devserver this is ~chromeos-test/images/
    @param keep_duration: How old of regular builds to keep around.
    @param keep_paladin_duration: How old of Paladin builds to keep around.
    """
    for timestamp_dir in get_all_timestamp_dirs(builds_dir):
        logging.debug('Processing %s', timestamp_dir)
        if '-paladin/' in timestamp_dir:
            keep = keep_paladin_duration
        else:
            keep = keep_duration
        if file_is_too_old(timestamp_dir, keep):
            logging.debug('Deleting %s', timestamp_dir)
            shutil.rmtree(timestamp_dir)
            # Resursively delete parent folders
            try_delete_parent_dir(timestamp_dir, builds_dir)


def main():
    """Main routine."""
    usage = 'usage: %prog [options] images_dir'
    parser = optparse.OptionParser(usage=usage)
    parser.add_option('-a', '--max-age', default=24, type=int,
                      help='Number of hours to keep normal builds: %default')
    parser.add_option('-p', '--max-paladin-age', default=24, type=int,
                      help='Number of hours to keep paladin builds: %default')
    parser.add_option('-v', '--verbose',
                      dest='verbose', action='store_true', default=False,
                      help='Run in verbose mode')
    options, args = parser.parse_args()
    if len(args) != 1:
        parser.print_usage()
        sys.exit(1)

    builds_dir = os.path.abspath(args[0])
    if not os.path.exists(builds_dir):
        logging.error('Builds dir %s does not exist', builds_dir)
        sys.exit(1)

    if options.verbose:
        logging.getLogger().setLevel(logging.DEBUG)
    else:
        logging.getLogger().setLevel(logging.INFO)

    prune_builds(builds_dir, options.max_age, options.max_paladin_age)


if __name__ == '__main__':
    main()
