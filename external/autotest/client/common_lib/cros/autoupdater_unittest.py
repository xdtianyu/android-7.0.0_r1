#!/usr/bin/python
# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import mox
import unittest

import common

import autoupdater
from autotest_lib.client.common_lib import error

class TestAutoUpdater(mox.MoxTestBase):
    """Test autoupdater module."""


    def testParseBuildFromUpdateUrlwithUpdate(self):
        """Test that we properly parse the build from an update_url."""
        update_url = ('http://172.22.50.205:8082/update/lumpy-release/'
                      'R27-3837.0.0')
        expected_value = 'lumpy-release/R27-3837.0.0'
        self.assertEqual(autoupdater.url_to_image_name(update_url),
                         expected_value)


    def testCheckVersion_1(self):
        """Test version check methods work for any build.

        Test two methods used to check version, check_version and
        check_version_to_confirm_install, for:
        1. trybot paladin build.
        update version: trybot-lumpy-paladin/R27-3837.0.0-b123
        booted version: 3837.0.2013_03_21_1340

        """
        update_url = ('http://172.22.50.205:8082/update/trybot-lumpy-paladin/'
                      'R27-1111.0.0-b123')
        updater = autoupdater.ChromiumOSUpdater(
                update_url, host=self.mox.CreateMockAnything())

        self.mox.UnsetStubs()
        self.mox.StubOutWithMock(updater.host, 'get_release_version')
        updater.host.get_release_version().MultipleTimes().AndReturn(
                                                    '1111.0.2013_03_21_1340')
        self.mox.ReplayAll()

        self.assertFalse(updater.check_version())
        self.assertTrue(updater.check_version_to_confirm_install())

        self.mox.UnsetStubs()
        self.mox.StubOutWithMock(updater.host, 'get_release_version')
        updater.host.get_release_version().MultipleTimes().AndReturn(
                '1111.0.0-rc1')
        self.mox.ReplayAll()

        self.assertFalse(updater.check_version())
        self.assertFalse(updater.check_version_to_confirm_install())

        self.mox.UnsetStubs()
        self.mox.StubOutWithMock(updater.host, 'get_release_version')
        updater.host.get_release_version().MultipleTimes().AndReturn('1111.0.0')
        self.mox.ReplayAll()

        self.assertFalse(updater.check_version())
        self.assertFalse(updater.check_version_to_confirm_install())

        self.mox.UnsetStubs()
        self.mox.StubOutWithMock(updater.host, 'get_release_version')
        updater.host.get_release_version().MultipleTimes().AndReturn(
                                                    '4444.0.0-pgo-generate')
        self.mox.ReplayAll()

        self.assertFalse(updater.check_version())
        self.assertFalse(updater.check_version_to_confirm_install())


    def testCheckVersion_2(self):
        """Test version check methods work for any build.

        Test two methods used to check version, check_version and
        check_version_to_confirm_install, for:
        2. trybot release build.
        update version: trybot-lumpy-release/R27-3837.0.0-b456
        booted version: 3837.0.0

        """
        update_url = ('http://172.22.50.205:8082/update/trybot-lumpy-release/'
                      'R27-2222.0.0-b456')
        updater = autoupdater.ChromiumOSUpdater(
                update_url, host=self.mox.CreateMockAnything())

        self.mox.UnsetStubs()
        self.mox.StubOutWithMock(updater.host, 'get_release_version')
        updater.host.get_release_version().MultipleTimes().AndReturn(
                                                    '2222.0.2013_03_21_1340')
        self.mox.ReplayAll()

        self.assertFalse(updater.check_version())
        self.assertFalse(updater.check_version_to_confirm_install())

        self.mox.UnsetStubs()
        self.mox.StubOutWithMock(updater.host, 'get_release_version')
        updater.host.get_release_version().MultipleTimes().AndReturn(
                '2222.0.0-rc1')
        self.mox.ReplayAll()

        self.assertFalse(updater.check_version())
        self.assertFalse(updater.check_version_to_confirm_install())

        self.mox.UnsetStubs()
        self.mox.StubOutWithMock(updater.host, 'get_release_version')
        updater.host.get_release_version().MultipleTimes().AndReturn('2222.0.0')
        self.mox.ReplayAll()

        self.assertFalse(updater.check_version())
        self.assertTrue(updater.check_version_to_confirm_install())

        self.mox.UnsetStubs()
        self.mox.StubOutWithMock(updater.host, 'get_release_version')
        updater.host.get_release_version().MultipleTimes().AndReturn(
                                                    '4444.0.0-pgo-generate')
        self.mox.ReplayAll()

        self.assertFalse(updater.check_version())
        self.assertFalse(updater.check_version_to_confirm_install())


    def testCheckVersion_3(self):
        """Test version check methods work for any build.

        Test two methods used to check version, check_version and
        check_version_to_confirm_install, for:
        3. buildbot official release build.
        update version: lumpy-release/R27-3837.0.0
        booted version: 3837.0.0

        """
        update_url = ('http://172.22.50.205:8082/update/lumpy-release/'
                      'R27-3333.0.0')
        updater = autoupdater.ChromiumOSUpdater(
                update_url, host=self.mox.CreateMockAnything())

        self.mox.UnsetStubs()
        self.mox.StubOutWithMock(updater.host, 'get_release_version')
        updater.host.get_release_version().MultipleTimes().AndReturn(
                                                    '3333.0.2013_03_21_1340')
        self.mox.ReplayAll()

        self.assertFalse(updater.check_version())
        self.assertFalse(updater.check_version_to_confirm_install())

        self.mox.UnsetStubs()
        self.mox.StubOutWithMock(updater.host, 'get_release_version')
        updater.host.get_release_version().MultipleTimes().AndReturn(
                '3333.0.0-rc1')
        self.mox.ReplayAll()

        self.assertFalse(updater.check_version())
        self.assertFalse(updater.check_version_to_confirm_install())

        self.mox.UnsetStubs()
        self.mox.StubOutWithMock(updater.host, 'get_release_version')
        updater.host.get_release_version().MultipleTimes().AndReturn('3333.0.0')
        self.mox.ReplayAll()

        self.assertTrue(updater.check_version())
        self.assertTrue(updater.check_version_to_confirm_install())

        self.mox.UnsetStubs()
        self.mox.StubOutWithMock(updater.host, 'get_release_version')
        updater.host.get_release_version().MultipleTimes().AndReturn(
                                                    '4444.0.0-pgo-generate')
        self.mox.ReplayAll()

        self.assertFalse(updater.check_version())
        self.assertFalse(updater.check_version_to_confirm_install())


    def testCheckVersion_4(self):
        """Test version check methods work for any build.

        Test two methods used to check version, check_version and
        check_version_to_confirm_install, for:
        4. non-official paladin rc build.
        update version: lumpy-paladin/R27-3837.0.0-rc7
        booted version: 3837.0.0-rc7

        """
        update_url = ('http://172.22.50.205:8082/update/lumpy-paladin/'
                      'R27-4444.0.0-rc7')
        updater = autoupdater.ChromiumOSUpdater(
                update_url, host=self.mox.CreateMockAnything())

        self.mox.UnsetStubs()
        self.mox.StubOutWithMock(updater.host, 'get_release_version')
        updater.host.get_release_version().MultipleTimes().AndReturn(
                                                    '4444.0.2013_03_21_1340')
        self.mox.ReplayAll()

        self.assertFalse(updater.check_version())
        self.assertFalse(updater.check_version_to_confirm_install())

        self.mox.UnsetStubs()
        self.mox.StubOutWithMock(updater.host, 'get_release_version')
        updater.host.get_release_version().MultipleTimes().AndReturn(
                '4444.0.0-rc7')
        self.mox.ReplayAll()

        self.assertTrue(updater.check_version())
        self.assertTrue(updater.check_version_to_confirm_install())

        self.mox.UnsetStubs()
        self.mox.StubOutWithMock(updater.host, 'get_release_version')
        updater.host.get_release_version().MultipleTimes().AndReturn('4444.0.0')
        self.mox.ReplayAll()

        self.assertFalse(updater.check_version())
        self.assertFalse(updater.check_version_to_confirm_install())

        self.mox.UnsetStubs()
        self.mox.StubOutWithMock(updater.host, 'get_release_version')
        updater.host.get_release_version().MultipleTimes().AndReturn(
                                                    '4444.0.0-pgo-generate')
        self.mox.ReplayAll()

        self.assertFalse(updater.check_version())
        self.assertFalse(updater.check_version_to_confirm_install())


    def testCheckVersion_5(self):
        """Test version check methods work for any build.

        Test two methods used to check version, check_version and
        check_version_to_confirm_install, for:
        5. chrome-perf build.
        update version: lumpy-chrome-perf/R28-3837.0.0-b2996
        booted version: 3837.0.0

        """
        update_url = ('http://172.22.50.205:8082/update/lumpy-chrome-perf/'
                      'R28-4444.0.0-b2996')
        updater = autoupdater.ChromiumOSUpdater(
                update_url, host=self.mox.CreateMockAnything())

        self.mox.UnsetStubs()
        self.mox.StubOutWithMock(updater.host, 'get_release_version')
        updater.host.get_release_version().MultipleTimes().AndReturn(
                                                    '4444.0.2013_03_21_1340')
        self.mox.ReplayAll()

        self.assertFalse(updater.check_version())
        self.assertFalse(updater.check_version_to_confirm_install())

        self.mox.UnsetStubs()
        self.mox.StubOutWithMock(updater.host, 'get_release_version')
        updater.host.get_release_version().MultipleTimes().AndReturn(
                '4444.0.0-rc7')
        self.mox.ReplayAll()

        self.assertFalse(updater.check_version())
        self.assertFalse(updater.check_version_to_confirm_install())

        self.mox.UnsetStubs()
        self.mox.StubOutWithMock(updater.host, 'get_release_version')
        updater.host.get_release_version().MultipleTimes().AndReturn('4444.0.0')
        self.mox.ReplayAll()

        self.assertFalse(updater.check_version())
        self.assertTrue(updater.check_version_to_confirm_install())

        self.mox.UnsetStubs()
        self.mox.StubOutWithMock(updater.host, 'get_release_version')
        updater.host.get_release_version().MultipleTimes().AndReturn(
                                                    '4444.0.0-pgo-generate')
        self.mox.ReplayAll()

        self.assertFalse(updater.check_version())
        self.assertFalse(updater.check_version_to_confirm_install())


    def testCheckVersion_6(self):
        """Test version check methods work for any build.

        Test two methods used to check version, check_version and
        check_version_to_confirm_install, for:
        6. pgo-generate build.
        update version: lumpy-release-pgo-generate/R28-3837.0.0-b2996
        booted version: 3837.0.0-pgo-generate

        """
        update_url = ('http://172.22.50.205:8082/update/lumpy-release-pgo-'
                      'generate/R28-4444.0.0-b2996')
        updater = autoupdater.ChromiumOSUpdater(
                update_url, host=self.mox.CreateMockAnything())

        self.mox.UnsetStubs()
        self.mox.StubOutWithMock(updater.host, 'get_release_version')
        updater.host.get_release_version().MultipleTimes().AndReturn(
                                                    '4444.0.0-2013_03_21_1340')
        self.mox.ReplayAll()

        self.assertFalse(updater.check_version())
        self.assertFalse(updater.check_version_to_confirm_install())

        self.mox.UnsetStubs()
        self.mox.StubOutWithMock(updater.host, 'get_release_version')
        updater.host.get_release_version().MultipleTimes().AndReturn(
                '4444.0.0-rc7')
        self.mox.ReplayAll()

        self.assertFalse(updater.check_version())
        self.assertFalse(updater.check_version_to_confirm_install())

        self.mox.UnsetStubs()
        self.mox.StubOutWithMock(updater.host, 'get_release_version')
        updater.host.get_release_version().MultipleTimes().AndReturn('4444.0.0')
        self.mox.ReplayAll()

        self.assertFalse(updater.check_version())
        self.assertFalse(updater.check_version_to_confirm_install())

        self.mox.UnsetStubs()
        self.mox.StubOutWithMock(updater.host, 'get_release_version')
        updater.host.get_release_version().MultipleTimes().AndReturn(
                                                    '4444.0.0-pgo-generate')
        self.mox.ReplayAll()

        self.assertFalse(updater.check_version())
        self.assertTrue(updater.check_version_to_confirm_install())


    def testCheckVersion_7(self):
        """Test version check methods work for a test-ap build.

        Test two methods used to check version, check_version and
        check_version_to_confirm_install, for:
        6. test-ap build.
        update version: trybot-stumpy-test-ap/R46-7298.0.0-b23
        booted version: 7298.0.0

        """
        update_url = ('http://100.107.160.2:8082/update/trybot-stumpy-test-api'
                      '/R46-7298.0.0-b23')
        updater = autoupdater.ChromiumOSUpdater(
                update_url, host=self.mox.CreateMockAnything())

        self.mox.UnsetStubs()
        self.mox.StubOutWithMock(updater.host, 'get_release_version')
        updater.host.get_release_version().MultipleTimes().AndReturn(
                '7298.0.2015_07_24_1640')
        self.mox.ReplayAll()

        self.assertFalse(updater.check_version())
        self.assertTrue(updater.check_version_to_confirm_install())

        self.mox.UnsetStubs()
        self.mox.StubOutWithMock(updater.host, 'get_release_version')
        updater.host.get_release_version().MultipleTimes().AndReturn(
                '7298.0.2015_07_24_1640')
        self.mox.ReplayAll()

        self.assertFalse(updater.check_version())
        self.assertTrue(updater.check_version_to_confirm_install())

        self.mox.UnsetStubs()
        self.mox.StubOutWithMock(updater.host, 'get_release_version')
        updater.host.get_release_version().MultipleTimes().AndReturn('7298.0.0')
        self.mox.ReplayAll()

        self.assertFalse(updater.check_version())
        self.assertFalse(updater.check_version_to_confirm_install())

        self.mox.UnsetStubs()
        self.mox.StubOutWithMock(updater.host, 'get_release_version')
        updater.host.get_release_version().MultipleTimes().AndReturn(
                '7298.0.0')
        self.mox.ReplayAll()

        self.assertFalse(updater.check_version())
        self.assertFalse(updater.check_version_to_confirm_install())


    def testTriggerUpdate(self):
        """Tests that we correctly handle updater errors."""
        update_url = 'http://server/test/url'
        host = self.mox.CreateMockAnything()
        self.mox.StubOutWithMock(host, 'run')
        host.hostname = 'test_host'

        expected_cmd = ('/usr/bin/update_engine_client --check_for_update '
                        '--omaha_url=http://server/test/url')

        updater = autoupdater.ChromiumOSUpdater(update_url, host=host)

        # Test with success.
        host.run(expected_cmd)

        # SSH Timeout
        host.run(expected_cmd).AndRaise(
                error.AutoservSSHTimeout("ssh timed out", 255))

        # SSH Permission Error
        host.run(expected_cmd).AndRaise(
                error.AutoservSshPermissionDeniedError("ssh timed out", 255))

        # Command Failed Error
        cmd_result_1 = self.mox.CreateMockAnything()
        cmd_result_1.exit_status = 1

        host.run(expected_cmd).AndRaise(
                error.AutoservRunError("ssh timed out", cmd_result_1))

        # Generic SSH Error (maybe)
        cmd_result_255 = self.mox.CreateMockAnything()
        cmd_result_255.exit_status = 255

        host.run(expected_cmd).AndRaise(
                error.AutoservRunError("Sometimes SSH specific result.",
                                       cmd_result_255))

        self.mox.ReplayAll()

        # Verify Success.
        updater.trigger_update()

        # Verify each type of error listed above.
        self.assertRaises(autoupdater.RootFSUpdateError,
                          updater.trigger_update)
        self.assertRaises(autoupdater.RootFSUpdateError,
                          updater.trigger_update)
        self.assertRaises(autoupdater.RootFSUpdateError,
                          updater.trigger_update)
        self.assertRaises(autoupdater.RootFSUpdateError,
                          updater.trigger_update)

        self.mox.VerifyAll()


    def testUpdateStateful(self):
        """Tests that we call the stateful update script with the correct args.
        """
        self.mox.StubOutWithMock(autoupdater.ChromiumOSUpdater, '_run')
        self.mox.StubOutWithMock(autoupdater.ChromiumOSUpdater,
                                 'get_stateful_update_script')
        update_url = ('http://172.22.50.205:8082/update/lumpy-chrome-perf/'
                      'R28-4444.0.0-b2996')
        static_update_url = ('http://172.22.50.205:8082/static/'
                             'lumpy-chrome-perf/R28-4444.0.0-b2996')

        # Test with clobber=False.
        autoupdater.ChromiumOSUpdater.get_stateful_update_script().AndReturn(
                autoupdater.ChromiumOSUpdater.REMOTE_STATEUL_UPDATE_PATH)
        autoupdater.ChromiumOSUpdater._run(
                mox.And(
                        mox.StrContains(
                                autoupdater.ChromiumOSUpdater.
                                REMOTE_STATEUL_UPDATE_PATH),
                        mox.StrContains(static_update_url),
                        mox.Not(mox.StrContains('--stateful_change=clean'))),
                timeout=mox.IgnoreArg())

        self.mox.ReplayAll()
        updater = autoupdater.ChromiumOSUpdater(update_url)
        updater.update_stateful(clobber=False)
        self.mox.VerifyAll()

        # Test with clobber=True.
        self.mox.ResetAll()
        autoupdater.ChromiumOSUpdater.get_stateful_update_script().AndReturn(
                autoupdater.ChromiumOSUpdater.REMOTE_STATEUL_UPDATE_PATH)
        autoupdater.ChromiumOSUpdater._run(
                mox.And(
                        mox.StrContains(
                                autoupdater.ChromiumOSUpdater.
                                REMOTE_STATEUL_UPDATE_PATH),
                        mox.StrContains(static_update_url),
                        mox.StrContains('--stateful_change=clean')),
                timeout=mox.IgnoreArg())
        self.mox.ReplayAll()
        updater = autoupdater.ChromiumOSUpdater(update_url)
        updater.update_stateful(clobber=True)
        self.mox.VerifyAll()


    def testRollbackRootfs(self):
        """Tests that we correctly rollback the rootfs when requested."""
        self.mox.StubOutWithMock(autoupdater.ChromiumOSUpdater, '_run')
        self.mox.StubOutWithMock(autoupdater.ChromiumOSUpdater,
                                 '_verify_update_completed')
        host = self.mox.CreateMockAnything()
        update_url = 'http://server/test/url'
        host.hostname = 'test_host'

        can_rollback_cmd = ('/usr/bin/update_engine_client --can_rollback')
        rollback_cmd = ('/usr/bin/update_engine_client --rollback '
                        '--follow')

        updater = autoupdater.ChromiumOSUpdater(update_url, host=host)

        # Return an old build which shouldn't call can_rollback.
        updater.host.get_release_version().AndReturn('1234.0.0')
        autoupdater.ChromiumOSUpdater._run(rollback_cmd)
        autoupdater.ChromiumOSUpdater._verify_update_completed()

        self.mox.ReplayAll()
        updater.rollback_rootfs(powerwash=True)
        self.mox.VerifyAll()

        self.mox.ResetAll()
        cmd_result_1 = self.mox.CreateMockAnything()
        cmd_result_1.exit_status = 1

        # Rollback but can_rollback says we can't -- return an error.
        updater.host.get_release_version().AndReturn('5775.0.0')
        autoupdater.ChromiumOSUpdater._run(can_rollback_cmd).AndRaise(
                error.AutoservRunError('can_rollback failed', cmd_result_1))
        self.mox.ReplayAll()
        self.assertRaises(autoupdater.RootFSUpdateError,
                          updater.rollback_rootfs, True)
        self.mox.VerifyAll()

        self.mox.ResetAll()
        # Rollback >= version blacklisted.
        updater.host.get_release_version().AndReturn('5775.0.0')
        autoupdater.ChromiumOSUpdater._run(can_rollback_cmd)
        autoupdater.ChromiumOSUpdater._run(rollback_cmd)
        autoupdater.ChromiumOSUpdater._verify_update_completed()
        self.mox.ReplayAll()
        updater.rollback_rootfs(powerwash=True)
        self.mox.VerifyAll()


if __name__ == '__main__':
  unittest.main()
