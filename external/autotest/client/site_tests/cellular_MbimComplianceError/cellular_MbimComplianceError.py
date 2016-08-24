# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os

import common
from autotest_lib.client.cros.cellular.mbim_compliance import mbim_test_runner

class cellular_MbimComplianceError(mbim_test_runner.MbimTestRunner):
    """
    Main test runner for all the tests within this directory. This just a
    harness for invoking various tests within this directory which is not
    currently supported by Autotest.

    """
    _TEST_AREA_FOLDER = os.path.dirname(__file__)
