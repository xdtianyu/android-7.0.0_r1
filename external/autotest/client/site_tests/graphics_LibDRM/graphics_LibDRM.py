# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
import logging

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import service_stopper

class graphics_LibDRM(test.test):
    version = 1
    _services = None

    def initialize(self):
        self._services = service_stopper.ServiceStopper(['ui'])

    def cleanup(self):
        if self._services:
           self._services.restore_services()

    def run_once(self):
        num_errors = 0
        keyvals = {}

        # These are tests to run for all platforms.
        tests_common = ['modetest']

        # Determine which tests to run based on the architecture type.
        tests_exynos5 = ['kmstest']
        tests_rockchip = ['kmstest']
        arch_tests = { 'arm'     : [],
                       'exynos5' : tests_exynos5,
                       'i386'    : [],
                       'rockchip': tests_rockchip,
                       'tegra'   : [],
                       'x86_64'  : [] }
        arch = utils.get_cpu_soc_family()
        if not arch in arch_tests:
            raise error.TestFail('Architecture "%s" not supported.', arch)
        elif arch == 'tegra':
            logging.warning('Tegra does not support DRM.')
            return
        tests = tests_common + arch_tests[arch]

        # If UI is running, we must stop it and restore later.
        self._services.stop_services()

        for test in tests:
            # Make sure the test exists on this system.  Not all tests may be
            # present on a given system.
            if utils.system('which %s' % test):
                logging.error('Could not find test %s.', test)
                keyvals[test] = 'NOT FOUND'
                num_errors += 1
                continue

            # Run the test and check for success based on return value.
            return_value = utils.system(test)
            if return_value:
                logging.error('%s returned %d', test, return_value)
                num_errors += 1
                keyvals[test] = 'FAILED'
            else:
                keyvals[test] = 'PASSED'

        self.write_perf_keyval(keyvals)

        if num_errors > 0:
            raise error.TestError('One or more libdrm tests failed.')
