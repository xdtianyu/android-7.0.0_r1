# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Module containing import helper used by autoupdate utility."""

import imp
import os

import common
from autotest_lib.utils import build_externals, external_packages


def download_and_import(module_name, package_class):
    """Tries to import module, if it fails, downloads and imports it.

    @param module_name: Name of the module e.g. devserver.
    @param package_class: autotest external_packages class to use.
    """
    try:
        return imp.load_module(module_name, *imp.find_module(module_name))
    except ImportError:
        pass

    tot = external_packages.find_top_of_autotest_tree()
    install_dir = os.path.join(tot, build_externals.INSTALL_DIR)
    build_externals.build_and_install_packages(
        [package_class], install_dir)

    return imp.load_module(module_name, *imp.find_module(module_name))
