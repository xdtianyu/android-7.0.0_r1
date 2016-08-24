# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

from lansim import tools


class ToolsTest(unittest.TestCase):
    """Unit tests for the tools."""


    def testInetHwton(self):
        """Tests inet_hwton."""
        self.assertEqual(tools.inet_hwton('\x12\x34\x56\x78\x90\xAB'),
                         '\x12\x34\x56\x78\x90\xAB')
        self.assertEqual(tools.inet_hwton('BA:C0:11:C0:FF:EE'),
                         '\xBA\xC0\x11\xC0\xFF\xEE')
        self.assertEqual(tools.inet_hwton('BAC011C0FFEE'),
                         '\xBA\xC0\x11\xC0\xFF\xEE')


    def testInetNtohw(self):
        """Tests inet_hwton."""
        self.assertEqual(tools.inet_ntohw('\xBA\xC0\x11\x00\x01\x0F'),
                        'BA:C0:11:00:01:0F'),


if __name__ == '__main__':
    unittest.main()
