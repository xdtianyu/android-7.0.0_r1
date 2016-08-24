#!/usr/bin/python
#
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unit tests for client/common_lib/cros/control_file_getter.py."""

import httplib
import logging
import mox
import StringIO
import unittest

import common

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import dev_server
from autotest_lib.server.cros.dynamic_suite import control_file_getter


class DevServerGetterTest(mox.MoxTestBase):
    """Unit tests for control_file_getter.DevServerGetter.

    @var _HOST: fake dev server host address.
    """

    _BUILD = 'fake/build'
    _FILES = ['a/b/control', 'b/c/control']
    _CONTENTS = 'Multi-line\nControl File Contents\n'
    _403 = dev_server.DevServerException('HTTP 403 Forbidden!')

    def setUp(self):
        super(DevServerGetterTest, self).setUp()
        self.dev_server = self.mox.CreateMock(dev_server.ImageServer)
        self.getter = control_file_getter.DevServerGetter(self._BUILD,
                                                          self.dev_server)


    def testListControlFiles(self):
        """Should successfully list control files from the dev server."""
        self.dev_server.list_control_files(
                self._BUILD,
                suite_name='').AndReturn(self._FILES)
        self.mox.ReplayAll()
        self.assertEquals(self.getter.get_control_file_list(), self._FILES)
        self.assertEquals(self.getter._files, self._FILES)


    def testListControlFilesFail(self):
        """Should fail to list control files from the dev server."""
        self.dev_server.list_control_files(
                self._BUILD,
                suite_name='').AndRaise(self._403)
        self.mox.ReplayAll()
        self.assertRaises(error.NoControlFileList,
                          self.getter.get_control_file_list)


    def testGetControlFile(self):
        """Should successfully get a control file from the dev server."""
        path = self._FILES[0]
        self.dev_server.get_control_file(self._BUILD,
                                         path).AndReturn(self._CONTENTS)
        self.mox.ReplayAll()
        self.assertEquals(self.getter.get_control_file_contents(path),
                          self._CONTENTS)


    def testGetControlFileFail(self):
        """Should fail to get a control file from the dev server."""
        path = self._FILES[0]
        self.dev_server.get_control_file(self._BUILD, path).AndRaise(self._403)
        self.mox.ReplayAll()
        self.assertRaises(error.ControlFileNotFound,
                          self.getter.get_control_file_contents,
                          path)


    def testGetControlFileByNameCached(self):
        """\
        Should successfully get a cf by name from the dev server, using a cache.
        """
        name = 'one'
        path = "file/%s/control" % name

        self.getter._files = self._FILES + [path]
        self.dev_server.get_control_file(self._BUILD,
                                         path).AndReturn(self._CONTENTS)
        self.mox.ReplayAll()
        self.assertEquals(self.getter.get_control_file_contents_by_name(name),
                          self._CONTENTS)


    def testGetControlFileByName(self):
        """\
        Should successfully get a control file from the dev server by name.
        """
        name = 'one'
        path = "file/%s/control" % name

        files = self._FILES + [path]
        self.dev_server.list_control_files(
                self._BUILD,
                suite_name='').AndReturn(files)
        self.dev_server.get_control_file(self._BUILD,
                                         path).AndReturn(self._CONTENTS)
        self.mox.ReplayAll()
        self.assertEquals(self.getter.get_control_file_contents_by_name(name),
                          self._CONTENTS)


    def testGetSuiteControlFileByName(self):
        """\
        Should successfully get a suite control file from the devserver by name.
        """
        name = 'control.bvt'
        path = "file/" + name

        files = self._FILES + [path]
        self.dev_server.list_control_files(
                self._BUILD,
                suite_name='').AndReturn(files)
        self.dev_server.get_control_file(self._BUILD,
                                         path).AndReturn(self._CONTENTS)
        self.mox.ReplayAll()
        self.assertEquals(self.getter.get_control_file_contents_by_name(name),
                          self._CONTENTS)


    def testGetControlFileByNameFail(self):
        """Should fail to get a control file from the dev server by name."""
        name = 'one'

        self.dev_server.list_control_files(
                self._BUILD,
                suite_name='').AndReturn(self._FILES)
        self.mox.ReplayAll()
        self.assertRaises(error.ControlFileNotFound,
                          self.getter.get_control_file_contents_by_name,
                          name)


if __name__ == '__main__':
    unittest.main()
