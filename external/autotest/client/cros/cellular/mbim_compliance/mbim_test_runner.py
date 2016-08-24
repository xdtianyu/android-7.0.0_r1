# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""
Module to be included by the main test executor file within each test area
of MBIM compliance suite. This harness is responsible for loading the
required test within an area and invoking it.

"""

import imp
import os

import common
from autotest_lib.client.bin import test
from autotest_lib.client.cros.cellular.mbim_compliance import mbim_errors


class MbimTestRunner(test.test):
    """
    Main test class which hooks up the MBIM compliance suite to autotest.
    It invokes the MBIM tests within an area of the MBIM compliance test suite.

    """
    _TEST_AREA_FOLDER = None

    version = 1

    def run_once(self, subtest_name, **kwargs):
        """
        Method invoked by autotest framework to start a test from the
        corresponding control file.

        @param subtest_name: Name of the compliance test to be invoked.
                The test has to be in the same folder as the control file and
                should have file_name and class_name as |subtest_name|.
        @param kwargs: Optional arguments which are passed to the actual test
                being run.

        """
        module_name = os.path.join(self._TEST_AREA_FOLDER, subtest_name + ".py")
        try:
            test_module = imp.load_source(subtest_name, module_name)
        except ImportError:
            mbim_errors.log_and_raise(mbim_errors.MBIMComplianceFrameworkError,
                                      'Test module %s not found', module_name)
        try:
            test_class = getattr(test_module, subtest_name)
        except AttributeError:
            mbim_errors.log_and_raise(mbim_errors.MBIMComplianceFrameworkError,
                                      'Test class %s not found', subtest_name)
        test = test_class()
        test.run_test(**kwargs)
