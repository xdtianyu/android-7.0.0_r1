#!/usr/bin/python
#
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unit tests for server/cros/dynamic_suite/tools.py."""

import mox
import unittest

import common
from autotest_lib.server.cros.dynamic_suite.fakes import FakeHost
from autotest_lib.server.cros.dynamic_suite.host_spec import HostSpec
from autotest_lib.server.cros.dynamic_suite import host_spec
from autotest_lib.server.cros.dynamic_suite import tools
from autotest_lib.server import frontend


class DynamicSuiteToolsTest(mox.MoxTestBase):
    """Unit tests for dynamic_suite tools module methods.

    @var _BOARD: fake board to reimage
    """

    _BOARD = 'board'
    _DEPENDENCIES = {'test1': ['label1'], 'test2': ['label2']}
    _POOL = 'bvt'

    def setUp(self):
        super(DynamicSuiteToolsTest, self).setUp()
        self.afe = self.mox.CreateMock(frontend.AFE)
        self.tko = self.mox.CreateMock(frontend.TKO)
        # Having these ordered by complexity is important!
        host_spec_list = [HostSpec([self._BOARD, self._POOL])]
        for dep_list in self._DEPENDENCIES.itervalues():
            host_spec_list.append(
                HostSpec([self._BOARD, self._POOL], dep_list))
        self.specs = host_spec.order_by_complexity(host_spec_list)

    def testInjectVars(self):
        """Should inject dict of varibles into provided strings."""
        def find_all_in(d, s):
            """Returns true if all key-value pairs in |d| are printed in |s|
            and the dictionary representation is also in |s|."""
            for k, v in d.iteritems():
                if isinstance(v, str):
                    if "%s='%s'\n" % (k, v) not in s:
                        return False
                else:
                    if "%s=%r\n" % (k, v) not in s:
                        return False
            args_dict_str = "%s=%s\n" % ('args_dict', repr(d))
            if args_dict_str not in s:
                return False
            return True

        v = {'v1': 'one', 'v2': 'two', 'v3': None, 'v4': False, 'v5': 5}
        self.assertTrue(find_all_in(v, tools.inject_vars(v, '')))
        self.assertTrue(find_all_in(v, tools.inject_vars(v, 'ctrl')))


    def testIncorrectlyLocked(self):
        """Should detect hosts locked by random users."""
        host = FakeHost(locked=True, locked_by='some guy')
        self.assertTrue(tools.incorrectly_locked(host))


    def testNotIncorrectlyLocked(self):
        """Should accept hosts locked by the infrastructure."""
        infra_user = 'an infra user'
        self.mox.StubOutWithMock(tools, 'infrastructure_user')
        tools.infrastructure_user().AndReturn(infra_user)
        self.mox.ReplayAll()
        host = FakeHost(locked=True, locked_by=infra_user)
        self.assertFalse(tools.incorrectly_locked(host))


if __name__ == "__main__":
    unittest.main()
