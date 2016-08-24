#!/usr/bin/python

# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os, shutil
from autotest_lib.client.bin import utils

version = 1


def setup(topdir):
    """Unpack tarball to src/ and apply patch.
    @param topdir: The directory of this deps.
    """
    tarball = 'webgl-performance-0.0.2.tar.bz2'
    srcdir = os.path.join(topdir, 'src')
    filesdir = os.path.join(topdir, 'files')
    shutil.rmtree(srcdir, ignore_errors=True)
    tarball_path = os.path.join(filesdir, tarball)
    if not os.path.exists(srcdir):
        os.mkdir(srcdir)
        utils.extract_tarball_to_dir(tarball_path, srcdir)
    os.chdir(srcdir)
    utils.system('patch -p1 < ../files/0001-Patch-index.html.patch')
    utils.system(
        'patch -p1 < ../files/0002-Always-increment-numberOfResults.patch')
    shutil.copy('../files/favicon.ico', srcdir)


pwd = os.getcwd()
utils.update_version(pwd + '/src', True, version, setup, pwd)
