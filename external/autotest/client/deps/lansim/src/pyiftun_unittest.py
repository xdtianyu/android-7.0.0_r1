# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

class PyIfTunTest(unittest.TestCase):
  """Simple tests to validate that pyiftun is compiled and installed."""

  def testModuleLoads(self):
    """Tests that the module loads.

    Since this module is compiled from C, there are cases where the module
    fails to load due to linkage problems.
    """
    from lansim import pyiftun

  def testConstantExpossed(self):
    """Tests at least one constant is expossed from the module."""
    from lansim import pyiftun
    self.assertTrue(hasattr(pyiftun, 'TUNSETIFF'))

if __name__ == '__main__':
  unittest.main()
