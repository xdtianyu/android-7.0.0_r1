# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This module contains unit tests for firmware_utils module."""

import unittest

import common_unittest_utils
import firmware_utils


class FirmwareUtilsTest(unittest.TestCase):
    """A class for firmware utils unit tests."""

    def test_get_fw_and_date(self):
        filenames = {
            # log directory names
            '20130422_020631-fw_1.0.170-manual':
                    ('fw_1.0.170', '20130422_020631'),
            '20130806_221321-fw_1.0.AA-robot':
                    ('fw_1.0.AA', '20130806_221321'),

            # gesture file names
            'rapid_taps_20.top_left-link-fw_1.0.AA-robot-20130806_223400.dat':
                    ('fw_1.0.AA', '20130806_223400'),
            'drumroll.fast-lumpy-fw_11.23-complete-20130710_063441.dat':
                    ('fw_11.23', '20130710_063441'),
        }

        for filename, (expected_fw, expected_date) in filenames.items():
            actual_fw, actual_date = firmware_utils.get_fw_and_date(filename)
            self.assertEqual(actual_fw, expected_fw)
            self.assertEqual(actual_date, expected_date)


if __name__ == '__main__':
  unittest.main()
