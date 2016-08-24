#!/usr/bin/python
#
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unit tests for site_utils/forgiving_config_parser.py."""

import logging, mox, os, tempfile, unittest
import forgiving_config_parser

class ForgivingConfigParserTest(mox.MoxTestBase):


    def setUp(self):
        super(ForgivingConfigParserTest, self).setUp()
        self._tmpconfig = tempfile.NamedTemporaryFile()


    def testReRead(self):
        """Test that we reread() loads the same config file over again."""
        section = 'first'
        option1 = 'option1'
        value1 = 'value1'
        option2 = 'option2'
        value2 = 'value2'

        # Create initial file.
        initial = forgiving_config_parser.ForgivingConfigParser()
        initial.add_section(section)
        initial.set(section, option1, value1)
        with open(self._tmpconfig.name, 'w') as conf_file:
            initial.write(conf_file)

        to_test = forgiving_config_parser.ForgivingConfigParser()
        to_test.read(self._tmpconfig.name)
        self.assertEquals(value1, to_test.getstring(section, option1))
        self.assertEquals(None, to_test.getstring(section, option2))


        initial.set(section, option2, value2)
        initial.remove_option(section, option1)
        with open(self._tmpconfig.name, 'w') as conf_file:
            initial.write(conf_file)

        to_test.reread()
        self.assertEquals(None, to_test.getstring(section, option1))
        self.assertEquals(value2, to_test.getstring(section, option2))


if __name__ == '__main__':
    unittest.main()
