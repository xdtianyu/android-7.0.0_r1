#!/usr/bin/python
# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unittests for deploy_production_local.py."""

from __future__ import print_function

import unittest

import deploy_production as deploy_production


class TestDeployProduction(unittest.TestCase):
    """Test deploy_production_local with commands mocked out."""

    def test_parse_arguments(self):
        """Test deploy_production_local.parse_arguments."""
        # No arguments.
        results = deploy_production.parse_arguments([])
        self.assertEqual(
                {'afe': 'cautotest', 'servers': [], 'args': [],
                 'cont': False, 'dryrun': False, 'verbose': False},
                vars(results))

        # Dryrun, continue
        results = deploy_production.parse_arguments(['--dryrun', '--continue'])
        self.assertDictContainsSubset(
                {'afe': 'cautotest', 'servers': [], 'args': [],
                 'cont': True, 'dryrun': True, 'verbose': False},
                vars(results))

        # List custom AFE server.
        results = deploy_production.parse_arguments(['--afe', 'foo'])
        self.assertDictContainsSubset(
                {'afe': 'foo', 'servers': [], 'args': [],
                 'cont': False, 'dryrun': False, 'verbose': False},
                vars(results))

        # List some servers
        results = deploy_production.parse_arguments(['foo', 'bar'])
        self.assertDictContainsSubset(
                {'afe': 'cautotest', 'servers': ['foo', 'bar'], 'args': [],
                 'cont': False, 'dryrun': False, 'verbose': False},
                vars(results))

        # List some local args
        results = deploy_production.parse_arguments(['--', 'foo', 'bar'])
        self.assertDictContainsSubset(
                {'afe': 'cautotest', 'servers': [], 'args': ['foo', 'bar'],
                 'cont': False, 'dryrun': False, 'verbose': False},
                vars(results))

        # List everything.
        results = deploy_production.parse_arguments(
                ['--continue', '--afe', 'foo', '--dryrun', 'foo', 'bar',
                 '--', '--actions-only', '--dryrun'])
        self.assertDictContainsSubset(
                {'afe': 'foo', 'servers': ['foo', 'bar'],
                 'args': ['--actions-only', '--dryrun'],
                 'cont': True, 'dryrun': True, 'verbose': False},
                vars(results))


if __name__ == '__main__':
    unittest.main()
