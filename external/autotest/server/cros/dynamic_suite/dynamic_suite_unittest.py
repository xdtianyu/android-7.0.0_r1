#!/usr/bin/python
#
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unit tests for server/cros/dynamic_suite/dynamic_suite.py."""

import mox
import os
import signal
import unittest

from autotest_lib.client.common_lib import base_job, error
from autotest_lib.client.common_lib.cros import dev_server
from autotest_lib.server.cros import provision
from autotest_lib.server.cros.dynamic_suite import dynamic_suite
from autotest_lib.server.cros.dynamic_suite.suite import Suite


class DynamicSuiteTest(mox.MoxTestBase):
    """Unit tests for dynamic_suite module methods.

    @var _DARGS: default args to vet.
    """

    _DEVSERVER_HOST = 'http://devserver1'
    _BUILDS = {provision.CROS_VERSION_PREFIX: 'build_1',
               provision.FW_RW_VERSION_PREFIX:'fwrw_build_1'}

    def setUp(self):

        super(DynamicSuiteTest, self).setUp()
        self._DARGS = {'name': 'name',
                       'builds': self._BUILDS,
                       'board': 'board',
                       'job': self.mox.CreateMock(base_job.base_job),
                       'num': 1,
                       'pool': 'pool',
                       'check_hosts': False,
                       'add_experimental': False,
                       'suite_dependencies': ['test_dep'],
                       'devserver_url': self._DEVSERVER_HOST}



    def testVetRequiredReimageAndRunArgs(self):
        """Should verify only that required args are present and correct."""
        spec = dynamic_suite.SuiteSpec(**self._DARGS)
        self.assertEquals(spec.builds, self._DARGS['builds'])
        self.assertEquals(spec.board, 'board:' + self._DARGS['board'])
        self.assertEquals(spec.name, self._DARGS['name'])
        self.assertEquals(spec.job, self._DARGS['job'])


    def testVetReimageAndRunBuildArgFail(self):
        """Should fail verification if both |builds| and |build| are not set.
        """
        self._DARGS['build'] = None
        self._DARGS['builds'] = None
        self.assertRaises(error.SuiteArgumentException,
                          dynamic_suite.SuiteSpec,
                          **self._DARGS)
        self._DARGS['build'] = 'build1'
        self._DARGS['builds'] = {'cros-version': 'build2',
                                 'firmware-version': 'build3'}
        self.assertRaises(error.SuiteArgumentException,
                          dynamic_suite.SuiteSpec,
                          **self._DARGS)


    def testVetReimageAndRunBoardArgFail(self):
        """Should fail verification because |board| arg is bad."""
        self._DARGS['board'] = None
        self.assertRaises(error.SuiteArgumentException,
                          dynamic_suite.SuiteSpec,
                          **self._DARGS)


    def testVetReimageAndRunNameArgFail(self):
        """Should fail verification because |name| arg is bad."""
        self._DARGS['name'] = None
        self.assertRaises(error.SuiteArgumentException,
                          dynamic_suite.SuiteSpec,
                          **self._DARGS)


    def testVetReimageAndRunJobArgFail(self):
        """Should fail verification because |job| arg is bad."""
        self._DARGS['job'] = None
        self.assertRaises(error.SuiteArgumentException,
                          dynamic_suite.SuiteSpec,
                          **self._DARGS)


    def testOverrideOptionalReimageAndRunArgs(self):
        """Should verify that optional args can be overridden."""
        spec = dynamic_suite.SuiteSpec(**self._DARGS)
        self.assertEquals(spec.pool, 'pool:' + self._DARGS['pool'])
        self.assertEquals(spec.num, self._DARGS['num'])
        self.assertEquals(spec.check_hosts, self._DARGS['check_hosts'])
        self.assertEquals(spec.add_experimental,
                          self._DARGS['add_experimental'])
        self.assertEquals(spec.suite_dependencies,
                          self._DARGS['suite_dependencies'])


    def testDefaultOptionalReimageAndRunArgs(self):
        """Should verify that optional args get defaults."""
        del(self._DARGS['pool'])
        del(self._DARGS['check_hosts'])
        del(self._DARGS['add_experimental'])
        del(self._DARGS['num'])
        del(self._DARGS['suite_dependencies'])

        spec = dynamic_suite.SuiteSpec(**self._DARGS)
        self.assertEquals(spec.pool, None)
        self.assertEquals(spec.num, None)
        self.assertEquals(spec.check_hosts, True)
        self.assertEquals(spec.add_experimental, True)
        self.assertEquals(spec.suite_dependencies, [])


    def testReimageAndSIGTERM(self):
        """Should reimage_and_run that causes a SIGTERM and fails cleanly."""
        def suicide(*_):
            """Send SIGTERM to current process to exit.

            @param _: Ignored.
            """
            os.kill(os.getpid(), signal.SIGTERM)

        # Mox doesn't play well with SIGTERM, but it does play well with
        # with exceptions, so here we're using an exception to simulate
        # execution being interrupted by a signal.
        class UnhandledSIGTERM(Exception):
            """Exception to be raised when SIGTERM is received."""
            pass

        def handler(signal_number, frame):
            """Handler for receiving a signal.

            @param signal_number: signal number.
            @param frame: stack frame object.
            """
            raise UnhandledSIGTERM()

        signal.signal(signal.SIGTERM, handler)
        spec = self.mox.CreateMock(dynamic_suite.SuiteSpec)
        spec.builds = self._BUILDS
        spec.test_source_build = Suite.get_test_source_build(self._BUILDS)
        spec.devserver = self.mox.CreateMock(dev_server.ImageServer)
        spec.devserver.stage_artifacts(
                spec.builds[provision.CROS_VERSION_PREFIX],
                ['control_files', 'test_suites']).WithSideEffects(suicide)
        spec.run_prod_code = False

        self.mox.ReplayAll()

        self.assertRaises(UnhandledSIGTERM,
                          dynamic_suite._perform_reimage_and_run,
                          spec, None, None, None)


if __name__ == '__main__':
    unittest.main()
