# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.server import test
from autotest_lib.site_utils import lxc


class ssp_PackageInstall(test.test):
    """Tests that server tests can install packages inside containers."""
    version = 1

    def install_os_packages(self, packages):
        """Install OS package in the test container.

        @param packages: OS packages to be installed.
        """
        for package in packages:
            logging.debug('Installing package %s...', package)
            lxc.install_package(package)


    def install_python_packages(self, packages):
        """Install python package in the test container.

        @param packages: Python packages to be installed.
        """
        for package in packages:
            logging.debug('Installing package %s...', package)
            lxc.install_python_package(package)


    def initialize(self):
        """Initialize test.
        """
        self.install_os_packages(['sox'])
        self.install_python_packages(['selenium'])


    def run_once(self):
        """There is no body for this test.

        @raise: error.TestError if the test is not running inside container or
                any of the given packages failed to be installed.

        """
        if not utils.is_in_container():
            raise error.TestError('Install OS package is only supported in '
                                  'server-side packaging.')
        # Test OS package can be used.
        utils.run('sox --help')
        logging.info('Found sox executable.')

        # Test python module can be used.
        from selenium import webdriver
        logging.info('Found webdriver at %s', webdriver.__file__)

        return