#!/usr/bin/python
#
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unit tests for client/common_lib/cros/dev_server.py."""

import httplib
import mox
import StringIO
import time
import unittest
import urllib2

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import global_config
from autotest_lib.client.common_lib.cros import dev_server
from autotest_lib.client.common_lib.cros import retry

def retry_mock(ExceptionToCheck, timeout_min):
    """A mock retry decorator to use in place of the actual one for testing.

    @param ExceptionToCheck: the exception to check.
    @param timeout_mins: Amount of time in mins to wait before timing out.

    """
    def inner_retry(func):
        """The actual decorator.

        @param func: Function to be called in decorator.

        """
        return func

    return inner_retry


class DevServerTest(mox.MoxTestBase):
    """Unit tests for dev_server.DevServer.

    @var _HOST: fake dev server host address.
    """

    _HOST = 'http://nothing'
    _CRASH_HOST = 'http://nothing-crashed'
    _CONFIG = global_config.global_config


    def setUp(self):
        super(DevServerTest, self).setUp()
        self.crash_server = dev_server.CrashServer(DevServerTest._CRASH_HOST)
        self.dev_server = dev_server.ImageServer(DevServerTest._HOST)
        self.android_dev_server = dev_server.AndroidBuildServer(
                DevServerTest._HOST)
        self.mox.StubOutWithMock(urllib2, 'urlopen')
        # Hide local restricted_subnets setting.
        dev_server.RESTRICTED_SUBNETS = []


    def testSimpleResolve(self):
        """One devserver, verify we resolve to it."""
        self.mox.StubOutWithMock(dev_server, '_get_dev_server_list')
        self.mox.StubOutWithMock(dev_server.DevServer, 'devserver_healthy')
        dev_server._get_dev_server_list().MultipleTimes().AndReturn(
                [DevServerTest._HOST])
        dev_server.DevServer.devserver_healthy(DevServerTest._HOST).AndReturn(
                                                                        True)
        self.mox.ReplayAll()
        devserver = dev_server.ImageServer.resolve('my_build')
        self.assertEquals(devserver.url(), DevServerTest._HOST)


    def testResolveWithFailure(self):
        """Ensure we rehash on a failed ping on a bad_host."""
        self.mox.StubOutWithMock(dev_server, '_get_dev_server_list')
        bad_host, good_host = 'http://bad_host:99', 'http://good_host:8080'
        dev_server._get_dev_server_list().MultipleTimes().AndReturn(
                [bad_host, good_host])

        # Mock out bad ping failure to bad_host by raising devserver exception.
        urllib2.urlopen(mox.StrContains(bad_host), data=None).AndRaise(
                dev_server.DevServerException())
        # Good host is good.
        to_return = StringIO.StringIO('{"free_disk": 1024}')
        urllib2.urlopen(mox.StrContains(good_host),
                        data=None).AndReturn(to_return)

        self.mox.ReplayAll()
        host = dev_server.ImageServer.resolve(0) # Using 0 as it'll hash to 0.
        self.assertEquals(host.url(), good_host)
        self.mox.VerifyAll()


    def testResolveWithFailureURLError(self):
        """Ensure we rehash on a failed ping on a bad_host after urlerror."""
        # Retry mock just return the original method.
        retry.retry = retry_mock
        self.mox.StubOutWithMock(dev_server, '_get_dev_server_list')
        bad_host, good_host = 'http://bad_host:99', 'http://good_host:8080'
        dev_server._get_dev_server_list().MultipleTimes().AndReturn(
                [bad_host, good_host])

        # Mock out bad ping failure to bad_host by raising devserver exception.
        urllib2.urlopen(mox.StrContains(bad_host),
                data=None).MultipleTimes().AndRaise(
                        urllib2.URLError('urlopen connection timeout'))

        # Good host is good.
        to_return = StringIO.StringIO('{"free_disk": 1024}')
        urllib2.urlopen(mox.StrContains(good_host),
                data=None).AndReturn(to_return)

        self.mox.ReplayAll()
        host = dev_server.ImageServer.resolve(0) # Using 0 as it'll hash to 0.
        self.assertEquals(host.url(), good_host)
        self.mox.VerifyAll()


    def testResolveWithManyDevservers(self):
        """Should be able to return different urls with multiple devservers."""
        self.mox.StubOutWithMock(dev_server.ImageServer, 'servers')
        self.mox.StubOutWithMock(dev_server.DevServer, 'devserver_healthy')

        host0_expected = 'http://host0:8080'
        host1_expected = 'http://host1:8082'

        dev_server.ImageServer.servers().MultipleTimes().AndReturn(
                [host0_expected, host1_expected])
        dev_server.DevServer.devserver_healthy(host0_expected).AndReturn(True)
        dev_server.DevServer.devserver_healthy(host1_expected).AndReturn(True)

        self.mox.ReplayAll()
        host0 = dev_server.ImageServer.resolve(0)
        host1 = dev_server.ImageServer.resolve(1)
        self.mox.VerifyAll()

        self.assertEqual(host0.url(), host0_expected)
        self.assertEqual(host1.url(), host1_expected)


    def _returnHttpServerError(self):
        e500 = urllib2.HTTPError(url='',
                                 code=httplib.INTERNAL_SERVER_ERROR,
                                 msg='',
                                 hdrs=None,
                                 fp=StringIO.StringIO('Expected.'))
        urllib2.urlopen(mox.IgnoreArg()).AndRaise(e500)


    def _returnHttpForbidden(self):
        e403 = urllib2.HTTPError(url='',
                                 code=httplib.FORBIDDEN,
                                 msg='',
                                 hdrs=None,
                                 fp=StringIO.StringIO('Expected.'))
        urllib2.urlopen(mox.IgnoreArg()).AndRaise(e403)


    def testSuccessfulTriggerDownloadSync(self):
        """Call the dev server's download method with synchronous=True."""
        name = 'fake/image'
        self.mox.StubOutWithMock(dev_server.ImageServer, '_finish_download')
        to_return = StringIO.StringIO('Success')
        urllib2.urlopen(mox.And(mox.StrContains(self._HOST),
                                mox.StrContains(name),
                                mox.StrContains('stage?'))).AndReturn(to_return)
        to_return = StringIO.StringIO('True')
        urllib2.urlopen(mox.And(mox.StrContains(self._HOST),
                                mox.StrContains(name),
                                mox.StrContains('is_staged'))).AndReturn(
                                                                      to_return)
        self.dev_server._finish_download(name, mox.IgnoreArg(), mox.IgnoreArg())

        # Synchronous case requires a call to finish download.
        self.mox.ReplayAll()
        self.dev_server.trigger_download(name, synchronous=True)
        self.mox.VerifyAll()


    def testSuccessfulTriggerDownloadASync(self):
        """Call the dev server's download method with synchronous=False."""
        name = 'fake/image'
        to_return = StringIO.StringIO('Success')
        urllib2.urlopen(mox.And(mox.StrContains(self._HOST),
                                mox.StrContains(name),
                                mox.StrContains('stage?'))).AndReturn(to_return)
        to_return = StringIO.StringIO('True')
        urllib2.urlopen(mox.And(mox.StrContains(self._HOST),
                                mox.StrContains(name),
                                mox.StrContains('is_staged'))).AndReturn(
                                                                      to_return)

        self.mox.ReplayAll()
        self.dev_server.trigger_download(name, synchronous=False)
        self.mox.VerifyAll()


    def testURLErrorRetryTriggerDownload(self):
        """Should retry on URLError, but pass through real exception."""
        self.mox.StubOutWithMock(time, 'sleep')

        refused = urllib2.URLError('[Errno 111] Connection refused')
        urllib2.urlopen(mox.IgnoreArg()).AndRaise(refused)
        time.sleep(mox.IgnoreArg())
        self._returnHttpForbidden()
        self.mox.ReplayAll()
        self.assertRaises(dev_server.DevServerException,
                          self.dev_server.trigger_download,
                          '')


    def testErrorTriggerDownload(self):
        """Should call the dev server's download method, fail gracefully."""
        self._returnHttpServerError()
        self.mox.ReplayAll()
        self.assertRaises(dev_server.DevServerException,
                          self.dev_server.trigger_download,
                          '')


    def testForbiddenTriggerDownload(self):
        """Should call the dev server's download method, get exception."""
        self._returnHttpForbidden()
        self.mox.ReplayAll()
        self.assertRaises(dev_server.DevServerException,
                          self.dev_server.trigger_download,
                          '')


    def testSuccessfulFinishDownload(self):
        """Should successfully call the dev server's finish download method."""
        name = 'fake/image'
        to_return = StringIO.StringIO('Success')
        urllib2.urlopen(mox.And(mox.StrContains(self._HOST),
                                mox.StrContains(name),
                                mox.StrContains('stage?'))).AndReturn(to_return)
        to_return = StringIO.StringIO('True')
        urllib2.urlopen(mox.And(mox.StrContains(self._HOST),
                                mox.StrContains(name),
                                mox.StrContains('is_staged'))).AndReturn(
                                                                      to_return)

        # Synchronous case requires a call to finish download.
        self.mox.ReplayAll()
        self.dev_server.finish_download(name)  # Raises on failure.
        self.mox.VerifyAll()


    def testErrorFinishDownload(self):
        """Should call the dev server's finish download method, fail gracefully.
        """
        self._returnHttpServerError()
        self.mox.ReplayAll()
        self.assertRaises(dev_server.DevServerException,
                          self.dev_server.finish_download,
                          '')


    def testListControlFiles(self):
        """Should successfully list control files from the dev server."""
        name = 'fake/build'
        control_files = ['file/one', 'file/two']
        to_return = StringIO.StringIO('\n'.join(control_files))
        urllib2.urlopen(mox.And(mox.StrContains(self._HOST),
                                mox.StrContains(name))).AndReturn(to_return)
        self.mox.ReplayAll()
        paths = self.dev_server.list_control_files(name)
        self.assertEquals(len(paths), 2)
        for f in control_files:
            self.assertTrue(f in paths)


    def testFailedListControlFiles(self):
        """Should call the dev server's list-files method, get exception."""
        self._returnHttpServerError()
        self.mox.ReplayAll()
        self.assertRaises(dev_server.DevServerException,
                          self.dev_server.list_control_files,
                          '')


    def testExplodingListControlFiles(self):
        """Should call the dev server's list-files method, get exception."""
        self._returnHttpForbidden()
        self.mox.ReplayAll()
        self.assertRaises(dev_server.DevServerException,
                          self.dev_server.list_control_files,
                          '')


    def testGetControlFile(self):
        """Should successfully get a control file from the dev server."""
        name = 'fake/build'
        file = 'file/one'
        contents = 'Multi-line\nControl File Contents\n'
        to_return = StringIO.StringIO(contents)
        urllib2.urlopen(mox.And(mox.StrContains(self._HOST),
                                mox.StrContains(name),
                                mox.StrContains(file))).AndReturn(to_return)
        self.mox.ReplayAll()
        self.assertEquals(self.dev_server.get_control_file(name, file),
                          contents)


    def testErrorGetControlFile(self):
        """Should try to get the contents of a control file, get exception."""
        self._returnHttpServerError()
        self.mox.ReplayAll()
        self.assertRaises(dev_server.DevServerException,
                          self.dev_server.get_control_file,
                          '', '')


    def testForbiddenGetControlFile(self):
        """Should try to get the contents of a control file, get exception."""
        self._returnHttpForbidden()
        self.mox.ReplayAll()
        self.assertRaises(dev_server.DevServerException,
                          self.dev_server.get_control_file,
                          '', '')


    def testGetLatestBuild(self):
        """Should successfully return a build for a given target."""
        self.mox.StubOutWithMock(dev_server.ImageServer, 'servers')
        self.mox.StubOutWithMock(dev_server.DevServer, 'devserver_healthy')

        dev_server.ImageServer.servers().AndReturn([self._HOST])
        dev_server.DevServer.devserver_healthy(self._HOST).AndReturn(True)

        target = 'x86-generic-release'
        build_string = 'R18-1586.0.0-a1-b1514'
        to_return = StringIO.StringIO(build_string)
        urllib2.urlopen(mox.And(mox.StrContains(self._HOST),
                                mox.StrContains(target))).AndReturn(to_return)
        self.mox.ReplayAll()
        build = dev_server.ImageServer.get_latest_build(target)
        self.assertEquals(build_string, build)


    def testGetLatestBuildWithManyDevservers(self):
        """Should successfully return newest build with multiple devservers."""
        self.mox.StubOutWithMock(dev_server.ImageServer, 'servers')
        self.mox.StubOutWithMock(dev_server.DevServer, 'devserver_healthy')

        host0_expected = 'http://host0:8080'
        host1_expected = 'http://host1:8082'

        dev_server.ImageServer.servers().MultipleTimes().AndReturn(
                [host0_expected, host1_expected])

        dev_server.DevServer.devserver_healthy(host0_expected).AndReturn(True)
        dev_server.DevServer.devserver_healthy(host1_expected).AndReturn(True)

        target = 'x86-generic-release'
        build_string1 = 'R9-1586.0.0-a1-b1514'
        build_string2 = 'R19-1586.0.0-a1-b3514'
        to_return1 = StringIO.StringIO(build_string1)
        to_return2 = StringIO.StringIO(build_string2)
        urllib2.urlopen(mox.And(mox.StrContains(host0_expected),
                                mox.StrContains(target))).AndReturn(to_return1)
        urllib2.urlopen(mox.And(mox.StrContains(host1_expected),
                                mox.StrContains(target))).AndReturn(to_return2)

        self.mox.ReplayAll()
        build = dev_server.ImageServer.get_latest_build(target)
        self.assertEquals(build_string2, build)


    def testCrashesAreSetToTheCrashServer(self):
        """Should send symbolicate dump rpc calls to crash_server."""
        self.mox.ReplayAll()
        call = self.crash_server.build_call('symbolicate_dump')
        self.assertTrue(call.startswith(self._CRASH_HOST))


    def _stageTestHelper(self, artifacts=[], files=[], archive_url=None):
        """Helper to test combos of files/artifacts/urls with stage call."""
        expected_archive_url = archive_url
        if not archive_url:
            expected_archive_url = 'gs://my_default_url'
            self.mox.StubOutWithMock(dev_server, '_get_image_storage_server')
            dev_server._get_image_storage_server().AndReturn(
                'gs://my_default_url')
            name = 'fake/image'
        else:
            # This is embedded in the archive_url. Not needed.
            name = ''

        to_return = StringIO.StringIO('Success')
        urllib2.urlopen(mox.And(mox.StrContains(expected_archive_url),
                                mox.StrContains(name),
                                mox.StrContains('artifacts=%s' %
                                                ','.join(artifacts)),
                                mox.StrContains('files=%s' % ','.join(files)),
                                mox.StrContains('stage?'))).AndReturn(to_return)
        to_return = StringIO.StringIO('True')
        urllib2.urlopen(mox.And(mox.StrContains(expected_archive_url),
                                mox.StrContains(name),
                                mox.StrContains('artifacts=%s' %
                                                ','.join(artifacts)),
                                mox.StrContains('files=%s' % ','.join(files)),
                                mox.StrContains('is_staged'))).AndReturn(
                                        to_return)

        self.mox.ReplayAll()
        self.dev_server.stage_artifacts(name, artifacts, files, archive_url)
        self.mox.VerifyAll()


    def testStageArtifactsBasic(self):
        """Basic functionality to stage artifacts (similar to trigger_download).
        """
        self._stageTestHelper(artifacts=['full_payload', 'stateful'])


    def testStageArtifactsBasicWithFiles(self):
        """Basic functionality to stage artifacts (similar to trigger_download).
        """
        self._stageTestHelper(artifacts=['full_payload', 'stateful'],
                              files=['taco_bell.coupon'])


    def testStageArtifactsOnlyFiles(self):
        """Test staging of only file artifacts."""
        self._stageTestHelper(files=['tasty_taco_bell.coupon'])


    def testStageWithArchiveURL(self):
        """Basic functionality to stage artifacts (similar to trigger_download).
        """
        self._stageTestHelper(files=['tasty_taco_bell.coupon'],
                              archive_url='gs://tacos_galore/my/dir')


    def testStagedFileUrl(self):
        """Sanity tests that the staged file url looks right."""
        devserver_label = 'x86-mario-release/R30-1234.0.0'
        url = self.dev_server.get_staged_file_url('stateful.tgz',
                                                  devserver_label)
        expected_url = '/'.join([self._HOST, 'static', devserver_label,
                                 'stateful.tgz'])
        self.assertEquals(url, expected_url)

        devserver_label = 'something_crazy/that/you_MIGHT/hate'
        url = self.dev_server.get_staged_file_url('chromiumos_image.bin',
                                                  devserver_label)
        expected_url = '/'.join([self._HOST, 'static', devserver_label,
                                 'chromiumos_image.bin'])
        self.assertEquals(url, expected_url)


    def _StageTimeoutHelper(self):
        """Helper class for testing staging timeout."""
        self.mox.StubOutWithMock(dev_server.ImageServer, 'call_and_wait')
        dev_server.ImageServer.call_and_wait(
                call_name='stage',
                artifacts=mox.IgnoreArg(),
                files=mox.IgnoreArg(),
                archive_url=mox.IgnoreArg(),
                error_message=mox.IgnoreArg()).AndRaise(error.TimeoutException)


    def test_StageArtifactsTimeout(self):
        """Test DevServerException is raised when stage_artifacts timed out."""
        self._StageTimeoutHelper()
        self.mox.ReplayAll()
        self.assertRaises(dev_server.DevServerException,
                          self.dev_server.stage_artifacts,
                          image='fake/image', artifacts=['full_payload'])
        self.mox.VerifyAll()


    def test_TriggerDownloadTimeout(self):
        """Test DevServerException is raised when trigger_download timed out."""
        self._StageTimeoutHelper()
        self.mox.ReplayAll()
        self.assertRaises(dev_server.DevServerException,
                          self.dev_server.trigger_download,
                          image='fake/image')
        self.mox.VerifyAll()


    def test_FinishDownloadTimeout(self):
        """Test DevServerException is raised when finish_download timed out."""
        self._StageTimeoutHelper()
        self.mox.ReplayAll()
        self.assertRaises(dev_server.DevServerException,
                          self.dev_server.finish_download,
                          image='fake/image')
        self.mox.VerifyAll()


    def test_compare_load(self):
        """Test load comparison logic.
        """
        load_high_cpu = {'devserver': 'http://devserver_1:8082',
                         dev_server.DevServer.CPU_LOAD: 100.0,
                         dev_server.DevServer.NETWORK_IO: 1024*1024*1.0,
                         dev_server.DevServer.DISK_IO: 1024*1024.0}
        load_high_network = {'devserver': 'http://devserver_1:8082',
                             dev_server.DevServer.CPU_LOAD: 1.0,
                             dev_server.DevServer.NETWORK_IO: 1024*1024*100.0,
                             dev_server.DevServer.DISK_IO: 1024*1024*1.0}
        load_1 = {'devserver': 'http://devserver_1:8082',
                  dev_server.DevServer.CPU_LOAD: 1.0,
                  dev_server.DevServer.NETWORK_IO: 1024*1024*1.0,
                  dev_server.DevServer.DISK_IO: 1024*1024*2.0}
        load_2 = {'devserver': 'http://devserver_1:8082',
                  dev_server.DevServer.CPU_LOAD: 1.0,
                  dev_server.DevServer.NETWORK_IO: 1024*1024*1.0,
                  dev_server.DevServer.DISK_IO: 1024*1024*1.0}
        self.assertFalse(dev_server._is_load_healthy(load_high_cpu))
        self.assertFalse(dev_server._is_load_healthy(load_high_network))
        self.assertTrue(dev_server._compare_load(load_1, load_2) > 0)


    def _testSuccessfulTriggerDownloadAndroid(self, synchronous=True):
        """Call the dev server's download method with given synchronous setting.

        @param synchronous: True to call the download method synchronously.
        """
        target = 'test_target'
        branch = 'test_branch'
        build_id = '123456'
        self.mox.StubOutWithMock(dev_server.AndroidBuildServer,
                                 '_finish_download')
        to_return = StringIO.StringIO('Success')
        urllib2.urlopen(mox.And(mox.StrContains(self._HOST),
                                mox.StrContains(target),
                                mox.StrContains(branch),
                                mox.StrContains(build_id),
                                mox.StrContains('stage?'))).AndReturn(to_return)
        to_return = StringIO.StringIO('True')
        urllib2.urlopen(mox.And(mox.StrContains(self._HOST),
                                mox.StrContains(target),
                                mox.StrContains(branch),
                                mox.StrContains(build_id),
                                mox.StrContains('is_staged'))).AndReturn(
                                                                      to_return)
        if synchronous:
            android_build_info = {'target': target,
                                  'build_id': build_id,
                                  'branch': branch}
            build = dev_server.ANDROID_BUILD_NAME_PATTERN % android_build_info
            self.android_dev_server._finish_download(
                    build,
                    dev_server._ANDROID_ARTIFACTS_TO_BE_STAGED_FOR_IMAGE, '',
                    target=target, build_id=build_id, branch=branch)

        # Synchronous case requires a call to finish download.
        self.mox.ReplayAll()
        self.android_dev_server.trigger_download(
                synchronous=synchronous, target=target, build_id=build_id,
                branch=branch)
        self.mox.VerifyAll()


    def testSuccessfulTriggerDownloadAndroidSync(self):
        """Call the dev server's download method with synchronous=True."""
        self._testSuccessfulTriggerDownloadAndroid(synchronous=True)


    def testSuccessfulTriggerDownloadAndroidAsync(self):
        """Call the dev server's download method with synchronous=False."""
        self._testSuccessfulTriggerDownloadAndroid(synchronous=False)


    def testGetUnrestrictedDevservers(self):
        """Test method get_unrestricted_devservers works as expected."""
        restricted_devserver = 'http://192.168.0.100:8080'
        unrestricted_devserver = 'http://172.1.1.3:8080'
        self.mox.StubOutWithMock(dev_server.ImageServer, 'servers')
        dev_server.ImageServer.servers().AndReturn([restricted_devserver,
                                                    unrestricted_devserver])
        self.mox.ReplayAll()
        self.assertEqual(dev_server.ImageServer.get_unrestricted_devservers(
                                [('192.168.0.0', 24)]),
                         [unrestricted_devserver])


if __name__ == "__main__":
    unittest.main()
