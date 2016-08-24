# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import errno
import os
import shutil
import urllib2

from autotest_lib.client.common_lib import global_config

def rm_dir_if_exists(dir_to_remove):
    """
    Removes a directory. Does not fail if the directory does NOT exist.

    @param dir_to_remove: path, directory to be removed.

    """
    try:
        shutil.rmtree(dir_to_remove)
    except OSError as e:
        if e.errno != errno.ENOENT:
            raise


def rm_dirs_if_exist(dirs_to_remove):
    """
    Removes multiple directories. Does not fail if directories do NOT exist.

    @param dirs_to_remove: list of directory paths to be removed.

    """
    for dr in dirs_to_remove:
        rm_dir_if_exists(dr)


def ensure_file_exists(filepath):
    """
    Verifies path given points to an existing file.

    @param filepath: path, path to check.

    @raises IOError if the path given does not point to a valid file.

    """
    error_msg = 'File %s does not exist.' % filepath
    if not os.path.isfile(filepath):
        raise IOError(error_msg)


def ensure_all_files_exist(filepaths):
    """
    Verifies all paths given point to existing files.

    @param filepaths: List of paths to check.

    @raises IOError if given paths do not point to existing files.

    """
    for filepath in filepaths:
        ensure_file_exists(filepath)


def ensure_dir_exists(dirpath):
    """
    Verifies path given points to an existing directory.

    @param dirpath: path, dir to check.

    @raises IOError if path does not point to an existing directory.

    """
    error_msg = 'Directory %s does not exist.' % dirpath
    if not os.path.isdir(dirpath):
        raise IOError(error_msg)


def ensure_all_dirs_exist(dirpaths):
    """
    Verifies all paths given point to existing directories.

    @param dirpaths: list of directory paths to check.

    @raises IOError if given paths do not point to existing directories.

    """
    for dirpath in dirpaths:
        ensure_dir_exists(dirpath)


def make_leaf_dir(dirpath):
    """
    Creates a directory, also creating parent directories if they do not exist.

    @param dirpath: path, directory to create.

    @raises whatever exception raised other than "path already exist".

    """
    try:
        os.makedirs(dirpath)
    except OSError as e:
        if e.errno != errno.EEXIST:
            raise


def make_leaf_dirs(dirpaths):
    """
    Creates multiple directories building all respective parent directories if
    they do not exist.

    @param dirpaths: list of directory paths to create.

    @raises whatever exception raised other than "path already exists".
    """
    for dirpath in dirpaths:
        make_leaf_dir(dirpath)


def download_file(remote_path, local_path):
    """
    Download file from a remote resource.

    @param remote_path: path, complete path to the remote file.
    @param local_path: path, complete path to save downloaded file.

    @raises: urllib2.HTTPError or urlib2.URLError exception. Both with added
            debug information

    """
    client_config = global_config.global_config.get_section_values('CLIENT')
    proxies = {}

    for name, value in client_config.items('CLIENT'):
        if value and name.endswith('_proxy'):
            proxies[name[:-6]] = value

    if proxies:
        proxy_handler = urllib2.ProxyHandler(proxies)
        opener = urllib2.build_opener(proxy_handler)
        urllib2.install_opener(opener)

    # Unlike urllib.urlopen urllib2.urlopen will immediately throw on error
    # If we could not find the file pointed by remote_path we will get an
    # exception, catch the exception to log useful information then re-raise

    try:
        remote_file = urllib2.urlopen(remote_path)

        # Catch exceptions, extract exception properties and then re-raise
        # This helps us with debugging what went wrong quickly as we get to see
        # test_that output immediately

    except urllib2.HTTPError as e:
        e.msg = (("""HTTPError raised while retrieving file %s\n.
                       Http Code = %s.\n. Reason = %s\n. Headers = %s.\n
                       Original Message = %s.\n""")
                 % (remote_path, e.code, e.reason, e.headers, e.msg))
        raise

    except urllib2.URLError as e:
        e.msg = (("""URLError raised while retrieving file %s\n.
                        Reason = %s\n. Original Message = %s\n.""")
                 % (remote_path, e.reason, e.msg))
        raise

    with open(local_path, 'wb') as local_file:
        local_file.write(remote_file.read())