#!/usr/bin/python

# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Short integration tests for full_release_test."""

import mox
import os
import tempfile
import shutil
import unittest

import common
from autotest_lib.site_utils.autoupdate import full_release_test
# This is convoluted because of the way we pull in gsutil_util.
# pylint: disable-msg=W0611
from autotest_lib.site_utils.autoupdate import test_image
from test_image import gsutil_util


def _ControlFileContainsLine(control_file, line):
    """Returns true if the string |control_file| contains line |line|."""
    for current_line in control_file.splitlines():
        if current_line == line:
            return True
    else:
        return False


def _DoesControlFileHaveSourceTarget(control_file_path, src, target):
    """Returns true if control file has src and target correctly filled in."""
    with open(control_file_path, 'r') as f:
        control_file = f.read()
        if not _ControlFileContainsLine(
                control_file, "source_release = '%s'" % src):
            print 'source_release does not match'
            return False

        if not _ControlFileContainsLine(
                control_file, "target_release = '%s'" % target):
          print 'target_release does not match'
          return False

    return True


class ParseVersionsTests(unittest.TestCase):
    """Tests various version parsing functions."""

    def setUp(self):
        self.config_gen = full_release_test.TestConfigGenerator

    def testParseBuildVersion(self):
        """Tests for _parse_build_version."""
        build_version = 'R27-3905.0.0'
        self.assertEquals(('R27', '3905.0.0'),
                          self.config_gen._parse_build_version(build_version))
        build_version = 'R41-6588.0.2014_12_16_1130-a1'
        self.assertEquals(('R41', '6588.0.2014_12_16_1130-a1'),
                          self.config_gen._parse_build_version(build_version))

    def testParseDeltaFilename(self):
        """"Tests for _parse_delta_filename."""
        delta_filename = 'chromeos_R27-390.0.0_R29-395.0.0_stumpy_delta_dev.bin'
        self.assertEquals(('R27-390.0.0', 'R29-395.0.0'),
                          self.config_gen._parse_delta_filename(delta_filename))

        # On non-release builds, delta filenames have a date portion as well.
        delta_filename = 'chromeos_R41-6588.0.2014_12_16_1130-a1_R41-6588.0.2014_12_16_1130-a1_stumpy_delta_dev.bin'
        self.assertEquals(('R41-6588.0.2014_12_16_1130-a1', 'R41-6588.0.2014_12_16_1130-a1'),
                          self.config_gen._parse_delta_filename(delta_filename))

class FullReleaseTestTests(mox.MoxTestBase):
    """Tests for the full_release_test.py test harness."""


    def setUp(self):
        """Common setUp creates tmpdir."""
        mox.MoxTestBase.setUp(self)
        self.mox.StubOutWithMock(gsutil_util, 'GSUtilRun')
        self.tmpdir = tempfile.mkdtemp('control')


    def tearDown(self):
        """Common tearDown removes tmpdir."""
        mox.MoxTestBase.tearDown(self)
        shutil.rmtree(self.tmpdir)


    def testIntegrationNmoBoard(self):
        """Tests that we successfully generate a nmo control file."""
        board = 'x86-mario'
        branch = '24'
        target = '3000.0.0'
        src = '2999.0.0'

        argv = ['--nmo',
                '--dump_dir', self.tmpdir,
                '--dump',
                target, board]

        # Return npo delta
        gsutil_util.GSUtilRun(mox.And(
                mox.StrContains('gsutil cat'),
                mox.StrContains('%s/UPLOADED' % target)), mox.IgnoreArg()).\
                AndReturn('chromeos_R%s-%s_R%s-%s_%s_delta_dev.bin' % (
                        branch, src, branch, target, board))
        # Return target full payload
        gsutil_util.GSUtilRun(mox.And(
                mox.StrContains('gsutil cat'),
                mox.StrContains('%s/UPLOADED' % src)), mox.IgnoreArg()).\
                AndReturn('chromeos_R%s-%s_%s_full_dev.bin' % (
                        branch, src, board))

        self.mox.ReplayAll()
        self.assertEquals(full_release_test.main(argv), 0)
        self.assertTrue(_DoesControlFileHaveSourceTarget(
                os.path.join(self.tmpdir, board, 'control.nmo_delta_%s' % src),
                src, target))
        self.mox.VerifyAll()


    def testIntegrationNpoBoard(self):
        """Tests that we successfully generate a npo control file."""
        board = 'x86-mario'
        branch = '24'
        target = '3000.0.0'
        src = '3000.0.0'

        argv = ['--npo',
                '--dump_dir', self.tmpdir,
                '--dump',
                target, board]

        # Return npo delta
        gsutil_util.GSUtilRun(mox.And(
                mox.StrContains('gsutil cat'),
                mox.StrContains('%s/UPLOADED' % target)), mox.IgnoreArg()).\
                AndReturn('chromeos_R%s-%s_R%s-%s_%s_delta_dev.bin' % (
                        branch, src, branch, target, board))
        # Return target full payload
        gsutil_util.GSUtilRun(mox.And(
                mox.StrContains('gsutil cat'),
                mox.StrContains('%s/UPLOADED' % src)), mox.IgnoreArg()).\
                AndReturn('chromeos_R%s-%s_%s_full_dev.bin' % (
                        branch, src, board))
        self.mox.ReplayAll()
        self.assertEquals(full_release_test.main(argv), 0)
        self.assertTrue(_DoesControlFileHaveSourceTarget(
                os.path.join(self.tmpdir, board, 'control.npo_delta_%s' % src),
                src, target))
        self.mox.VerifyAll()


    def testIntegrationNpoWithArchiveUrl(self):
        """Successfully generate a npo control file with custom url."""
        board = 'x86-mario'
        branch = '24'
        target = '3000.0.0'
        src = '3000.0.0'
        archive_url = 'gs://chromeos-image-archive/blah-dir/not_a_version'

        argv = ['--npo',
                '--dump_dir', self.tmpdir,
                '--dump',
                '--archive_url', archive_url,
                target, board]

        # Return npo delta
        gsutil_util.GSUtilRun(mox.And(
                mox.StrContains('gsutil cat'),
                mox.StrContains(archive_url)), mox.IgnoreArg()).\
                AndReturn('chromeos_R%s-%s_R%s-%s_%s_delta_dev.bin' % (
                        branch, src, branch, target, board))
        # Return target full payload
        gsutil_util.GSUtilRun(mox.And(
                mox.StrContains('gsutil cat'),
                mox.StrContains(archive_url)), mox.IgnoreArg()).\
                AndReturn('chromeos_R%s-%s_%s_full_dev.bin' % (
                        branch, src, board))
        self.mox.ReplayAll()
        self.assertEquals(full_release_test.main(argv), 0)
        self.assertTrue(_DoesControlFileHaveSourceTarget(
                os.path.join(self.tmpdir, board, 'control.npo_delta_%s' % src),
                src, target))
        self.mox.VerifyAll()


    def testIntegrationNpoAllBoards(self):
        """Tests that we successfully generate a npo control file 4 all boards.
        """
        boards = ['stumpy', 'lumpy', 'bumpy']
        branch = '24'
        target = '3000.0.0'
        src = '3000.0.0'

        argv = ['--npo',
                '-n',
                target] + boards

        for board in boards:
            # Return npo delta
            gsutil_util.GSUtilRun(mox.And(
                    mox.StrContains('gsutil cat'),
                    mox.StrContains('%s/UPLOADED' % target)), mox.IgnoreArg()).\
                    AndReturn('chromeos_R%s-%s_R%s-%s_%s_delta_dev.bin' % (
                            branch, src, branch, target, board))
            # Return target full payload
            gsutil_util.GSUtilRun(mox.And(
                    mox.StrContains('gsutil cat'),
                    mox.StrContains('%s/UPLOADED' % src)), mox.IgnoreArg()).\
                    AndReturn('chromeos_R%s-%s_%s_full_dev.bin' % (
                            branch, src, board))

        self.mox.ReplayAll()
        self.assertEquals(full_release_test.main(argv), 0)
        self.mox.VerifyAll()

        self.mox.ResetAll()
        # Verify we still run all of them even if one fails.
        bad_board = 'stumpy'
        for board in boards:
            # Return npo delta
            if board == bad_board:
                gsutil_util.GSUtilRun(mox.And(
                        mox.StrContains('gsutil cat'),
                        mox.StrContains('%s/UPLOADED' % target)),
                                        mox.IgnoreArg()).\
                        AndReturn('NO DELTAS FOR YOU')
                continue

            gsutil_util.GSUtilRun(mox.And(
                    mox.StrContains('gsutil cat'),
                    mox.StrContains('%s/UPLOADED' % target)), mox.IgnoreArg()).\
                    AndReturn('chromeos_R%s-%s_R%s-%s_%s_delta_dev.bin' % (
                            branch, src, branch, target, board))

            # Return target full payload
            gsutil_util.GSUtilRun(mox.And(
                    mox.StrContains('gsutil cat'),
                    mox.StrContains('%s/UPLOADED' % src)), mox.IgnoreArg()).\
                    AndReturn('chromeos_R%s-%s_%s_full_dev.bin' % (
                            branch, src, board))

        self.mox.ReplayAll()
        self.assertEquals(full_release_test.main(argv), 0)
        self.mox.VerifyAll()


    def testIntegrationSpecificBoard(self):
        """Tests that we successfully generate a specific control file."""
        board = 'x86-mario'
        branch = '24'
        target = '3000.0.0'
        src = '1234.0.0'

        argv = ['--specific', src,
                '--dump_dir', self.tmpdir,
                '--dump',
                target, board]

        # Return target full payload
        gsutil_util.GSUtilRun(mox.And(
                mox.StrContains('gsutil cat'),
                mox.StrContains('%s/UPLOADED' % target)), mox.IgnoreArg()).\
                AndReturn('chromeos_R%s-%s_%s_full_dev.bin' % (
                        branch, target, board))
        # Return src full payload
        gsutil_util.GSUtilRun(mox.And(
                mox.StrContains('gsutil cat'),
                mox.StrContains('%s/UPLOADED' % src)), mox.IgnoreArg()).\
                AndReturn('chromeos_R%s-%s_%s_full_dev.bin' % (
                        branch, src, board))
        self.mox.ReplayAll()
        self.assertEquals(full_release_test.main(argv), 0)
        self.assertTrue(_DoesControlFileHaveSourceTarget(
                os.path.join(self.tmpdir, board,
                             'control.specific_full_%s' % src),
                src, target))
        self.mox.VerifyAll()


    def testIntegrationSpecificBoardFail(self):
        """Tests we don't generate a specific test if either payload missing."""
        board = 'x86-mario'
        branch = '24'
        target = '3000.0.0'
        src = '1234.0.0'

        argv = ['--specific', src,
                '--dump_dir', self.tmpdir,
                '--dump',
                target, board]

        # Return target full payload
        gsutil_util.GSUtilRun(mox.And(
                mox.StrContains('gsutil cat'),
                mox.StrContains('%s/UPLOADED' % target)), mox.IgnoreArg()).\
                AndReturn('chromeos_R%s-%s_%s_full_dev.bin' % (
                        branch, target, board))
        # No src full payload
        gsutil_util.GSUtilRun(mox.And(
                mox.StrContains('gsutil cat'),
                mox.StrContains('%s/UPLOADED' % src)), mox.IgnoreArg()).\
                AndReturn('SOME OTHER DATA')
        self.mox.ReplayAll()
        self.assertEquals(full_release_test.main(argv), 1)
        self.mox.VerifyAll()

        self.mox.ResetAll()
        # Return target full payload
        gsutil_util.GSUtilRun(mox.And(
                mox.StrContains('gsutil cat'),
                mox.StrContains('%s/UPLOADED' % target)), mox.IgnoreArg()).\
                AndReturn('SOME OTHER DATA')
        self.mox.ReplayAll()
        self.assertEquals(full_release_test.main(argv), 1)
        self.mox.VerifyAll()


if __name__ == '__main__':
    unittest.main()
