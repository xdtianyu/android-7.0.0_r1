#!/usr/bin/python
#
# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, mox, subprocess, unittest

import common

from autotest_lib.frontend.health import check_test_health


class RunPrepScriptsTests(mox.MoxTestBase):
    """Test the run_prep_scripts() function."""

    def setUp(self):
        super(RunPrepScriptsTests, self).setUp()
        self.mox.StubOutWithMock(subprocess, 'call')
        self.mox.StubOutWithMock(logging, 'error')
        self.mox.StubOutWithMock(logging, 'info')


    def test_given_scripts_are_called(self):
        """Test that all the scripts passed in are called when they pass."""
        scripts = [['script1.sh', 'arg'], ['script2.sh']]

        logging.info(mox.IgnoreArg(), mox.IgnoreArg())
        subprocess.call(['script1.sh', 'arg']).AndReturn(0)
        logging.info(mox.IgnoreArg(), mox.IgnoreArg())
        subprocess.call(['script2.sh']).AndReturn(0)

        self.mox.ReplayAll()
        check_test_health.run_prep_scripts(scripts)


    def test_return_true_if_all_scripts_suceed(self):
        """Test that True is returned when all the scripts succeed."""
        scripts = [['script.sh']]

        logging.info(mox.IgnoreArg(), mox.IgnoreArg())
        subprocess.call(mox.IgnoreArg()).AndReturn(0)

        self.mox.ReplayAll()
        self.assertTrue(check_test_health.run_prep_scripts(scripts))


    def test_script_information_logging(self):
        """Test that we log prep running and failure."""
        scripts = [['pass.py'], ['fail.sh', 'arg']]

        logging.info('Running %s', 'pass.py')
        subprocess.call(['pass.py']).AndReturn(0)
        logging.info('Running %s', 'fail.sh arg')
        subprocess.call(['fail.sh', 'arg']).AndReturn(1)
        logging.error('\'%s\' failed with return code %d',
                      ('fail.sh arg', 1))

        self.mox.ReplayAll()
        check_test_health.run_prep_scripts(scripts)


    def test_return_false_if_script_fails(self):
        """Test that False is returned if a preparation step fails."""
        scripts = [['script.sh']]

        logging.info(mox.IgnoreArg(), mox.IgnoreArg())
        subprocess.call(mox.IgnoreArg()).AndReturn(1)
        logging.error(mox.IgnoreArg(), mox.IgnoreArg())

        self.mox.ReplayAll()
        self.assertFalse(check_test_health.run_prep_scripts(scripts))


    def test_do_not_run_other_scripts_after_one_fails(self):
        """Test that the other prep scripts are not ran if one fails."""
        scripts = [['script1.sh', 'arg'], ['script2.sh']]

        logging.info(mox.IgnoreArg(), mox.IgnoreArg())
        subprocess.call(['script1.sh', 'arg']).AndReturn(1)
        logging.error(mox.IgnoreArg(), mox.IgnoreArg())

        self.mox.ReplayAll()
        check_test_health.run_prep_scripts(scripts)



class RunAnalysisScripts(mox.MoxTestBase):
    """Test the run_analysis_scripts() function."""

    def setUp(self):
        super(RunAnalysisScripts, self).setUp()
        self.mox.StubOutWithMock(subprocess, 'call')
        self.mox.StubOutWithMock(logging, 'error')
        self.mox.StubOutWithMock(logging, 'info')


    def test_given_scripts_are_called(self):
        """Test that all the scripts passed in are called when they pass."""
        scripts = [['script1.sh', 'arg'], ['script2.sh']]

        logging.info(mox.IgnoreArg(), mox.IgnoreArg())
        subprocess.call(['script1.sh', 'arg']).AndReturn(0)
        logging.info(mox.IgnoreArg(), mox.IgnoreArg())
        subprocess.call(['script2.sh']).AndReturn(0)

        self.mox.ReplayAll()
        check_test_health.run_analysis_scripts(scripts)


    def test_return_true_if_all_scripts_suceed(self):
        """Test that True is returned when all the scripts succeed."""
        scripts = [['script.sh']]

        logging.info(mox.IgnoreArg(), mox.IgnoreArg())
        subprocess.call(mox.IgnoreArg()).AndReturn(0)

        self.mox.ReplayAll()
        self.assertTrue(check_test_health.run_analysis_scripts(scripts))


    def test_script_information_logging(self):
        """Test that we log prep running and failure."""
        scripts = [['pass.py'], ['fail.sh', 'arg']]

        logging.info('Running %s', 'pass.py')
        subprocess.call(['pass.py']).AndReturn(0)
        logging.info('Running %s', 'fail.sh arg')
        subprocess.call(['fail.sh', 'arg']).AndReturn(1)
        logging.error('\'%s\' failed with return code %d',
                      ('fail.sh arg', 1))

        self.mox.ReplayAll()
        check_test_health.run_analysis_scripts(scripts)


    def test_return_false_if_script_fails(self):
        """"Test that False is returned when at least one script fails."""
        scripts = [['script.sh']]

        logging.info(mox.IgnoreArg(), mox.IgnoreArg())
        subprocess.call(mox.IgnoreArg()).AndReturn(1)
        logging.error(mox.IgnoreArg(), mox.IgnoreArg())

        self.mox.ReplayAll()
        self.assertFalse(check_test_health.run_analysis_scripts(scripts))


    def test_run_other_scripts_after_one_fails(self):
        """Test that the other analysis scripts are ran even if one fails."""
        scripts = [['script1.sh', 'arg'], ['script2.sh']]

        logging.info(mox.IgnoreArg(), mox.IgnoreArg())
        subprocess.call(['script1.sh', 'arg']).AndReturn(1)
        logging.error(mox.IgnoreArg(), mox.IgnoreArg())
        logging.info(mox.IgnoreArg(), mox.IgnoreArg())
        subprocess.call(['script2.sh']).AndReturn(0)

        self.mox.ReplayAll()
        check_test_health.run_analysis_scripts(scripts)


class MainTests(mox.MoxTestBase):
    """Tests the main function."""

    def setUp(self):
        super(MainTests, self).setUp()
        self.mox.StubOutWithMock(check_test_health, 'run_prep_scripts')
        self.mox.StubOutWithMock(check_test_health, 'run_analysis_scripts')
        self._orig_prep = check_test_health.PREP_SCRIPTS
        self._orig_analysis = check_test_health.ANALYSIS_SCRIPTS


    def tearDown(self):
        super(MainTests, self).tearDown()
        check_test_health.PREP_SCRIPTS = self._orig_prep
        check_test_health.ANALYSIS_SCRIPTS = self._orig_analysis


    def test_all_functions_called_if_there_are_no_errors(self):
        """Test that all the script calling functions are called by default."""
        check_test_health.PREP_SCRIPTS = [['test_prep']]
        check_test_health.ANALYSIS_SCRIPTS = [['test_analysis']]

        check_test_health.run_prep_scripts(
            check_test_health.PREP_SCRIPTS).AndReturn(True)
        check_test_health.run_analysis_scripts(
            check_test_health.ANALYSIS_SCRIPTS).AndReturn(True)

        self.mox.ReplayAll()
        self.assertEqual(check_test_health.main(), 0)


    def test_handle_prep_failure(self):
        """Test that we properly handle a prep script failing."""
        check_test_health.run_prep_scripts(mox.IgnoreArg()).AndReturn(False)

        self.mox.ReplayAll()
        self.assertEqual(check_test_health.main(), 1)


    def test_handle_analysis_failure(self):
        """Test that we properly handle an analysis script failing."""
        check_test_health.run_prep_scripts(mox.IgnoreArg()).AndReturn(True)
        check_test_health.run_analysis_scripts(mox.IgnoreArg()).AndReturn(False)

        self.mox.ReplayAll()
        self.assertEqual(check_test_health.main(), 1)


if __name__ == '__main__':
    unittest.main()
