#!/usr/bin/python
# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import __builtin__
import mox
import os
import unittest
import time
from StringIO import StringIO

import utils


class TestUtils(mox.MoxTestBase):
    """Test utility functions."""


    def test_load_servo_interface_mapping(self):
        """Test servo-interface mapping file can be loaded."""
        self.mox.StubOutWithMock(__builtin__, 'open')
        fake_content = (
                'chromeos1-rack5-host10-servo, chromeos1-poe-switch1, fa42\n'
                'chromeos1-rack5-host11-servo, chromeos1-poe-switch1, fa43\n'
                ', chromeos2-poe-switch8, fa43\n'
                'chromeos2-rack5-host11-servo, chromeos2-poe-switch8, fa44\n')
        fake_file = self.mox.CreateMockAnything()
        fake_file.__enter__().AndReturn(StringIO(fake_content))
        fake_file.__exit__(mox.IgnoreArg(), mox.IgnoreArg(), mox.IgnoreArg())
        open('fake_file.csv').AndReturn(fake_file)
        expect = {'chromeos1-rack5-host10-servo':
                          ('chromeos1-poe-switch1', 'fa42'),
                  'chromeos1-rack5-host11-servo':
                          ('chromeos1-poe-switch1', 'fa43'),
                  'chromeos2-rack5-host11-servo':
                          ('chromeos2-poe-switch8', 'fa44')}
        self.mox.ReplayAll()
        self.assertEqual(
                utils.load_servo_interface_mapping('fake_file.csv'), expect)
        self.mox.VerifyAll()


    def _reload_helper(self, do_reload):
        """Helper class for mapping file reloading tests."""
        self.mox.StubOutWithMock(utils, 'load_servo_interface_mapping')
        self.mox.StubOutWithMock(os.path, 'getmtime')
        check_point = 1369783561.8525634
        if do_reload:
            last_modified = check_point + 10.0
            servo_interface = {'fake_servo': ('fake_switch', 'fake_if')}
            utils.load_servo_interface_mapping('fake_file').AndReturn(
                    servo_interface)
        else:
            last_modified = check_point
        os.path.getmtime(mox.IgnoreArg()).AndReturn(last_modified)
        self.mox.ReplayAll()
        result = utils.reload_servo_interface_mapping_if_necessary(
                check_point, mapping_file='fake_file')
        if do_reload:
            self.assertEqual(result, (last_modified, servo_interface))
        else:
            self.assertIsNone(result)
        self.mox.VerifyAll()


    def test_reload_servo_interface_mapping_necessary(self):
        """Test that mapping file is reloaded when it is modified."""
        self._reload_helper(True)


    def test_reload_servo_interface_mapping_not_necessary(self):
        """Test that mapping file is not reloaded when it is not modified."""
        self._reload_helper(False)


    def  test_LRU_cache(self):
        """Test LRUCache."""
        p1 = utils.PowerUnitInfo(
                'host1', utils.PowerUnitInfo.POWERUNIT_TYPES.RPM,
                'rpm1', 'hydra1')
        p2 = utils.PowerUnitInfo(
                'host2', utils.PowerUnitInfo.POWERUNIT_TYPES.RPM,
                'rpm2', 'hydra2')
        p3 = utils.PowerUnitInfo(
                'host3', utils.PowerUnitInfo.POWERUNIT_TYPES.RPM,
                'rpm3', 'hydra3')
        # Initialize an LRU with size 2, items never expire.
        cache = utils.LRUCache(2, expiration_secs=None)
        # Add two items, LRU should be full now
        cache['host1'] = p1
        cache['host2'] = p2
        self.assertEqual(len(cache.cache), 2)
        # Visit host2 and add one more item
        # host1 should be removed from cache
        _ = cache['host2']
        cache['host3'] = p3
        self.assertEqual(len(cache.cache), 2)
        self.assertTrue('host1' not in cache)
        self.assertTrue('host2' in cache)
        self.assertTrue('host3' in cache)


    def  test_LRU_cache_expires(self):
        """Test LRUCache expires."""
        self.mox.StubOutWithMock(time, 'time')
        time.time().AndReturn(10)
        time.time().AndReturn(25)
        p1 = utils.PowerUnitInfo(
                'host1', utils.PowerUnitInfo.POWERUNIT_TYPES.RPM,
                'rpm1', 'hydra1')

        self.mox.ReplayAll()
        # Initialize an LRU with size 1, items exppire after 10 secs.
        cache = utils.LRUCache(1, expiration_secs=10)
        # Add two items, LRU should be full now
        cache['host1'] = p1
        check_contains_1 = 'host1' in cache
        check_contains_2 = 'host2' in cache
        self.mox.VerifyAll()
        self.assertFalse(check_contains_1)
        self.assertFalse(check_contains_2)


    def  test_LRU_cache_full_with_expries(self):
        """Test timestamp is removed properly when cache is full."""
        self.mox.StubOutWithMock(time, 'time')
        time.time().AndReturn(10)
        time.time().AndReturn(25)
        p1 = utils.PowerUnitInfo(
                'host1', utils.PowerUnitInfo.POWERUNIT_TYPES.RPM,
                'rpm1', 'hydra1')
        p2 = utils.PowerUnitInfo(
                'host2', utils.PowerUnitInfo.POWERUNIT_TYPES.RPM,
                'rpm2', 'hydra2')
        self.mox.ReplayAll()
        # Initialize an LRU with size 1, items expire after 10 secs.
        cache = utils.LRUCache(1, expiration_secs=10)
        # Add two items, LRU should be full now
        cache['host1'] = p1
        cache['host2'] = p2
        self.mox.VerifyAll()
        self.assertEqual(len(cache.timestamps), 1)
        self.assertEqual(len(cache.cache), 1)
        self.assertTrue('host2' in cache.timestamps)
        self.assertTrue('host2' in cache.cache)


if __name__ == '__main__':
    unittest.main()
