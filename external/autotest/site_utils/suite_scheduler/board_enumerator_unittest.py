#!/usr/bin/python
#
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unit tests for site_utils/board_enumerator.py."""

import logging
import mox
import unittest

import board_enumerator

from autotest_lib.server import frontend
from constants import Labels


class BoardEnumeratorTest(mox.MoxTestBase):
    """Unit tests for BoardEnumerator."""


    def setUp(self):
        super(BoardEnumeratorTest, self).setUp()
        self.afe = self.mox.CreateMock(frontend.AFE)
        self.enumerator = board_enumerator.BoardEnumerator(afe=self.afe)
        self.prefix = Labels.BOARD_PREFIX


    def _CreateMockLabel(self, name):
        """Creates a mock frontend.Label, with the given name."""
        mock = self.mox.CreateMock(frontend.Label)
        mock.name = name
        return mock


    def testEnumerateBoards(self):
        """Test successful board enumeration."""
        labels = ['board1', 'board2', 'board3']
        self.afe.get_labels(name__startswith=self.prefix).AndReturn(
            map(lambda p: self._CreateMockLabel(self.prefix+p), labels))
        self.mox.ReplayAll()
        self.assertEquals(labels, self.enumerator.Enumerate())


    def testEnumerateNoBoards(self):
        """Test successful board enumeration, but there are no boards."""
        self.afe.get_labels(name__startswith=self.prefix).AndReturn([])
        self.mox.ReplayAll()
        self.assertRaises(board_enumerator.NoBoardException,
                          self.enumerator.Enumerate)


    def testEnumerateBoardsExplodes(self):
        """Listing boards raises an exception from the AFE."""
        self.afe.get_labels(name__startswith=self.prefix).AndRaise(Exception())
        self.mox.ReplayAll()
        self.assertRaises(board_enumerator.EnumerateException,
                          self.enumerator.Enumerate)


if __name__ == '__main__':
    unittest.main()
