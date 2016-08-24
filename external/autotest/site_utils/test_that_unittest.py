#!/usr/bin/python
# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
# pylint: disable-msg=C0111

import unittest
import common
from autotest_lib.site_utils import test_that


class TestThatUnittests(unittest.TestCase):
    def test_validate_arguments(self):
        # Deferred until validate_arguments allows for lab runs.
        pass

    def test_parse_arguments(self):
        args = test_that.parse_arguments(
                ['-b', 'some_board', '-i', 'some_image', '--args', 'some_args',
                 'some_remote', 'test1', 'test2'])
        self.assertEqual('some_board', args.board)
        self.assertEqual('some_image', args.build)
        self.assertEqual('some_args', args.args)
        self.assertEqual('some_remote', args.remote)
        self.assertEqual(['test1', 'test2'], args.tests)

    def test_parse_arguments_internal(self):
        args, remote_argv = test_that._parse_arguments_internal(
                ['-b', 'some_board', '-i', 'some_image', '--args', 'some_args',
                 'some_remote', 'test1', 'test2'])
        self.assertEqual('some_board', args.board)
        self.assertEqual('some_image', args.build)
        self.assertEqual('some_args', args.args)
        self.assertEqual('some_remote', args.remote)
        self.assertEqual(['test1', 'test2'], args.tests)
        self.assertEqual(remote_argv,
                         ['-b', 'some_board', '-i', 'some_image', '--args',
                          'some_args', 'some_remote', 'test1', 'test2'])

    def test_parse_arguments_internal_with_local_argument(self):
        args, remote_argv = test_that._parse_arguments_internal(
                ['-b', 'some_board', '-i', 'some_image', '-w', 'server:port',
                 '--args', 'some_args', 'some_remote', 'test1', 'test2'])
        self.assertEqual('server:port', args.web)
        self.assertEqual('some_board', args.board)
        self.assertEqual('some_image', args.build)
        self.assertEqual('some_args', args.args)
        self.assertEqual('some_remote', args.remote)
        self.assertEqual(['test1', 'test2'], args.tests)
        self.assertEqual(remote_argv,
                         ['-b', 'some_board', '-i', 'some_image', '--args',
                          'some_args', 'some_remote', 'test1', 'test2'])

    def test_parse_arguments_with_local_argument(self):
        args = test_that.parse_arguments(
                ['-b', 'some_board', '-i', 'some_image', '-w', 'server:port',
                 '--args', 'some_args', 'some_remote', 'test1', 'test2'])
        self.assertEqual('server:port', args.web)
        self.assertEqual('some_board', args.board)
        self.assertEqual('some_image', args.build)
        self.assertEqual('some_args', args.args)
        self.assertEqual('some_remote', args.remote)
        self.assertEqual(['test1', 'test2'], args.tests)

    def test_fetch_local_suite(self):
        # Deferred until fetch_local_suite knows about non-local builds.
        pass


if __name__ == '__main__':
    unittest.main()
