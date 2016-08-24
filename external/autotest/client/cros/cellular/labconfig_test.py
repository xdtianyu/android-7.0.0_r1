#!/usr/bin/env python
# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# pylint: disable-msg=C0111

import unittest
import cellular
import labconfig
# Use the same import line to keep this global on the same key
from autotest_lib.client.cros.cellular import labconfig_data


TEST_CELL = {
    'duts': [
        {
            'address': '1.2.3.4',
            'name': 'one_two_three_four',
            'technologies': ['CDMA_2000'],
            'rf_switch_port': 0
            },
        {
            'address': '5.6.7.8',
            'name': 'five_six_seven_eight',
            'technologies': ['GPRS', 'EGPRS'],
            },
        ],
    'rf_switch': {
        'type': 'ether_io',
        'address':  '172.31.206.172',
        }
    }

class TestLabConfig(unittest.TestCase):
    def setUp(self):
        # Monkey-patch in our test cell
        labconfig_data.CELLS['test'] = TEST_CELL

    def test_get_present_cell(self):
        c = labconfig.Configuration(['--cell', 'test'])

    def test_get_missing_cell(self):
        self.assertRaises(labconfig.LabConfigError,
                          labconfig.Configuration, ['--cell', 'NOT_PRESENT'])

    def test_get_dut(self):
        c = labconfig.Configuration(['--cell', 'test'])
        m = c._get_dut('1.2.3.4')
        self.assertEqual('one_two_three_four', m['name'])

        m = c._get_dut('one_two_three_four')
        self.assertEqual('one_two_three_four', m['name'])

    def test_get_technologies(self):
        c = labconfig.Configuration(['--cell', 'test', '--technology=all'])
        t = c.get_technologies('five_six_seven_eight')
        self.assertEqual([cellular.Technology.GPRS, cellular.Technology.EGPRS],
                         t)

        c = labconfig.Configuration(['--cell=test',
                                     '--technology=WCDMA,CDMA_2000'])

        self.assertEqual(
            [cellular.Technology.WCDMA, cellular.Technology.CDMA_2000],
            c.get_technologies('five_six_seven_eight'))

    def test_get_interface_ip(self):
        self.assertEqual('127.0.0.1', labconfig.get_interface_ip('lo'))

    def test_get_rf_switch_port(self):
        c = labconfig.Configuration(['--cell', 'test', '--technology=all'])
        self.assertEqual(0,
                         c.get_rf_switch_port('one_two_three_four'))

if __name__ == '__main__':
  unittest.main()
