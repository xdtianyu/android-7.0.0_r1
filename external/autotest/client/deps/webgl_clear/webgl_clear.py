#!/usr/bin/python

# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os, shutil
from autotest_lib.client.bin import utils

version = 1

def setup(topdir):
    """Download TDL library tarball and unpack to src/, then
    install remaining files/ into src/.
    @param topdir: The directory of this deps.
    """
    tarball = 'tdl-0.0.2.tar.gz'
    srcdir = os.path.join(topdir, 'src')
    filesdir = os.path.join(topdir, 'files')
    tarball_path = os.path.join(filesdir, tarball)

    shutil.rmtree(srcdir, ignore_errors=True)

    if not os.path.exists(tarball_path):
        utils.get_file(
            'http://github.com/greggman/tdl/archive/0.0.2.tar.gz', tarball_path)

    os.mkdir(srcdir)
    utils.extract_tarball_to_dir(tarball_path, srcdir)
    os.chdir(srcdir)
    shutil.copy(os.path.join(filesdir, 'WebGLClear.html'), srcdir)

pwd = os.getcwd()
utils.update_version(pwd + '/src', True, version, setup, pwd)
