# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#

"""This dummy module import all unit tests.

This file is named as xxx_unittests.py so that it will not get picked up
by the autotest-wide unittest running tool (utils/unittest_suite.py).
"""

import unittest

from firmware_summary_unittest import *
from geometry_unittest import *
from mtb_unittest import *
from validators_unittest import *


if __name__ == '__main__':
    unittest.main()
