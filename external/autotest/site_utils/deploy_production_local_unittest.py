#!/usr/bin/python
# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unittests for deploy_production_local.py."""

from __future__ import print_function

import mock
import subprocess
import unittest

import deploy_production_local as dpl


class TestDeployProductionLocal(unittest.TestCase):
    """Test deploy_production_local with commands mocked out."""

    orig_timer = dpl.SERVICE_STABILITY_TIMER

    PROD_STATUS = ('\x1b[1mproject autotest/                             '
                   '  \x1b[m\x1b[1mbranch prod\x1b[m\n')

    PROD_VERSIONS = '''\x1b[1mproject autotest/\x1b[m
/usr/local/autotest
ebb2182

\x1b[1mproject autotest/site_utils/autotest_private/\x1b[m
/usr/local/autotest/site_utils/autotest_private
78b9626

\x1b[1mproject autotest/site_utils/autotest_tools/\x1b[m
/usr/local/autotest/site_utils/autotest_tools
a1598f7
'''


    def setUp(self):
        dpl.SERVICE_STABILITY_TIMER = 0.01

    def tearDown(self):
        dpl.SERVICE_STABILITY_TIMER = self.orig_timer

    def test_strip_terminal_codes(self):
        """Test deploy_production_local.strip_terminal_codes."""
        # Leave format free lines alone.
        result = dpl.strip_terminal_codes('')
        self.assertEqual(result, '')

        result = dpl.strip_terminal_codes('This is normal text.')
        self.assertEqual(result, 'This is normal text.')

        result = dpl.strip_terminal_codes('Line1\nLine2\n')
        self.assertEqual(result, 'Line1\nLine2\n')

        result = dpl.strip_terminal_codes('Line1\nLine2\n')
        self.assertEqual(result, 'Line1\nLine2\n')

        # Test cleaning lines with formatting.
        result = dpl.strip_terminal_codes('\x1b[1m')
        self.assertEqual(result, '')

        result = dpl.strip_terminal_codes('\x1b[m')
        self.assertEqual(result, '')

        result = dpl.strip_terminal_codes('\x1b[1mm')
        self.assertEqual(result, 'm')

        result = dpl.strip_terminal_codes(self.PROD_STATUS)
        self.assertEqual(result,
                'project autotest/                               branch prod\n')

        result = dpl.strip_terminal_codes(self.PROD_VERSIONS)
        self.assertEqual(result, '''project autotest/
/usr/local/autotest
ebb2182

project autotest/site_utils/autotest_private/
/usr/local/autotest/site_utils/autotest_private
78b9626

project autotest/site_utils/autotest_tools/
/usr/local/autotest/site_utils/autotest_tools
a1598f7
''')

    @mock.patch('subprocess.check_output', autospec=True)
    def test_verify_repo_clean(self, run_cmd):
        """Test deploy_production_local.verify_repo_clean.

        @param run_cmd: Mock of subprocess call used.
        """
        # If repo returns what we expect, exit cleanly.
        run_cmd.return_value = 'nothing to commit (working directory clean)\n'
        dpl.verify_repo_clean()

        # If repo contains any branches (even clean ones), raise.
        run_cmd.return_value = self.PROD_STATUS
        with self.assertRaises(dpl.DirtyTreeException):
            dpl.verify_repo_clean()

        # If repo doesn't return what we expect, raise.
        run_cmd.return_value = "That's a very dirty repo you've got."
        with self.assertRaises(dpl.DirtyTreeException):
            dpl.verify_repo_clean()

    @mock.patch('subprocess.check_output', autospec=True)
    def test_repo_versions(self, run_cmd):
        """Test deploy_production_local.repo_versions.

        @param run_cmd: Mock of subprocess call used.
        """
        expected = {
            'autotest':
            ('/usr/local/autotest', 'ebb2182'),
            'autotest/site_utils/autotest_private':
            ('/usr/local/autotest/site_utils/autotest_private', '78b9626'),
            'autotest/site_utils/autotest_tools':
            ('/usr/local/autotest/site_utils/autotest_tools', 'a1598f7'),
        }

        run_cmd.return_value = self.PROD_VERSIONS
        result = dpl.repo_versions()
        self.assertEquals(result, expected)

        run_cmd.assert_called_with(
                ['repo', 'forall', '-p', '-c',
                 'pwd && git log -1 --format=%h'])

    @mock.patch('subprocess.check_output', autospec=True)
    def test_repo_sync(self, run_cmd):
        """Test deploy_production_local.repo_sync.

        @param run_cmd: Mock of subprocess call used.
        """
        dpl.repo_sync()
        run_cmd.assert_called_with(['repo', 'sync'])

    def test_discover_commands_and_services(self):
        """Test deploy_production_local.discover_update_commands and
        discover_restart_services."""
        # It should always be a list, and should always be callable in
        # any local environment, though the result will vary.
        result = dpl.discover_update_commands()
        self.assertIsInstance(result, list)

        result = dpl.discover_restart_services()
        self.assertIsInstance(result, list)

    @mock.patch('subprocess.check_output', autospec=True)
    def test_update_command(self, run_cmd):
        """Test deploy_production_local.update_command.

        @param run_cmd: Mock of subprocess call used.
        """
        # Call with a bad command name.
        with self.assertRaises(dpl.UnknownCommandException):
            dpl.update_command('Unknown Command')
        self.assertFalse(run_cmd.called)

        # Call with a valid command name.
        dpl.update_command('apache')
        run_cmd.assert_called_with('sudo service apache2 reload', shell=True,
                                   stderr=subprocess.STDOUT)

        # Call with a valid command name that uses AUTOTEST_REPO expansion.
        dpl.update_command('build_externals')
        expanded_cmd = dpl.common.autotest_dir+'/utils/build_externals.py'
        run_cmd.assert_called_with(expanded_cmd, shell=True,
                                   stderr=subprocess.STDOUT)

        # Test a failed command.
        failure = subprocess.CalledProcessError(10, expanded_cmd, 'output')

        run_cmd.side_effect = failure
        with self.assertRaises(subprocess.CalledProcessError) as unstable:
            dpl.update_command('build_externals')

    @mock.patch('subprocess.check_call', autospec=True)
    def test_restart_service(self, run_cmd):
        """Test deploy_production_local.restart_service.

        @param run_cmd: Mock of subprocess call used.
        """
        # Standard call.
        dpl.restart_service('foobar')
        run_cmd.assert_called_with(['sudo', 'service', 'foobar', 'restart'])

    @mock.patch('subprocess.check_output', autospec=True)
    def test_restart_status(self, run_cmd):
        """Test deploy_production_local.service_status.

        @param run_cmd: Mock of subprocess call used.
        """
        # Standard call.
        dpl.service_status('foobar')
        run_cmd.assert_called_with(['sudo', 'status', 'foobar'])

    @mock.patch.object(dpl, 'restart_service', autospec=True)
    def _test_restart_services(self, service_results, _restart):
        """Helper for testing restart_services.

        @param service_results: {'service_name': ['status_1', 'status_2']}
        """
        # each call to service_status should return the next status value for
        # that service.
        with mock.patch.object(dpl, 'service_status', autospec=True,
                               side_effect=lambda n: service_results[n].pop(0)):
            dpl.restart_services(service_results.keys())

    def test_restart_services(self):
        """Test deploy_production_local.restart_services."""
        single_stable = {'foo': ['status_ok', 'status_ok']}
        double_stable = {'foo': ['status_a', 'status_a'],
                         'bar': ['status_b', 'status_b']}

        # Verify we can handle stable services.
        self._test_restart_services(single_stable)
        self._test_restart_services(double_stable)

        single_unstable = {'foo': ['status_ok', 'status_not_ok']}
        triple_unstable = {'foo': ['status_a', 'status_a'],
                           'bar': ['status_b', 'status_b_not_ok'],
                           'joe': ['status_c', 'status_c_not_ok']}

        # Verify we can handle unstable services and report the right failures.
        with self.assertRaises(dpl.UnstableServices) as unstable:
            self._test_restart_services(single_unstable)
        self.assertEqual(unstable.exception.args[0], ['foo'])

        with self.assertRaises(dpl.UnstableServices) as unstable:
            self._test_restart_services(triple_unstable)
        self.assertEqual(unstable.exception.args[0], ['bar', 'joe'])

    @mock.patch('subprocess.check_output', autospec=True)
    def test_report_changes_no_update(self, run_cmd):
        """Test deploy_production_local.report_changes.

        @param run_cmd: Mock of subprocess call used.
        """

        before = {
            'autotest': ('/usr/local/autotest', 'auto_before'),
            'autotest_private': ('/dir/autotest_private', '78b9626'),
            'other': ('/fake/unchanged', 'constant_hash'),
        }

        run_cmd.return_value = 'hash1 Fix change.\nhash2 Bad change.\n'

        result = dpl.report_changes(before, None)

        self.assertEqual(result, """autotest: auto_before
autotest_private: 78b9626
other: constant_hash
""")

        self.assertFalse(run_cmd.called)

    @mock.patch('subprocess.check_output', autospec=True)
    def test_report_changes_diff(self, run_cmd):
        """Test deploy_production_local.report_changes.

        @param run_cmd: Mock of subprocess call used.
        """

        before = {
            'autotest': ('/usr/local/autotest', 'auto_before'),
            'autotest_private': ('/dir/autotest_private', '78b9626'),
            'other': ('/fake/unchanged', 'constant_hash'),
        }

        after = {
            'autotest': ('/usr/local/autotest', 'auto_after'),
            'autotest_tools': ('/dir/autotest_tools', 'a1598f7'),
            'other': ('/fake/unchanged', 'constant_hash'),
        }

        run_cmd.return_value = 'hash1 Fix change.\nhash2 Bad change.\n'

        result = dpl.report_changes(before, after)

        self.assertEqual(result, """autotest:
hash1 Fix change.
hash2 Bad change.

autotest_private:
Removed.

autotest_tools:
Added.

other:
No Change.
""")

        run_cmd.assert_called_with(
                ['git', 'log', 'auto_before..auto_after', '--oneline'],
                cwd='/usr/local/autotest', stderr=subprocess.STDOUT)

    def test_parse_arguments(self):
        """Test deploy_production_local.parse_arguments."""
        # No arguments.
        results = dpl.parse_arguments([])
        self.assertDictContainsSubset(
                {'verify': True, 'update': True, 'actions': True,
                 'report': True, 'dryrun': False},
                vars(results))

        # Dryrun.
        results = dpl.parse_arguments(['--dryrun'])
        self.assertDictContainsSubset(
                {'verify': False, 'update': False, 'actions': True,
                 'report': True, 'dryrun': True},
                vars(results))

        # Restart only.
        results = dpl.parse_arguments(['--actions-only'])
        self.assertDictContainsSubset(
                {'verify': False, 'update': False, 'actions': True,
                 'report': False, 'dryrun': False},
                vars(results))

        # All skip arguments.
        results = dpl.parse_arguments(['--skip-verify', '--skip-update',
                                       '--skip-actions', '--skip-report'])
        self.assertDictContainsSubset(
                {'verify': False, 'update': False, 'actions': False,
                 'report': False, 'dryrun': False},
                vars(results))

        # All arguments.
        results = dpl.parse_arguments(['--skip-verify', '--skip-update',
                                       '--skip-actions', '--skip-report',
                                       '--actions-only', '--dryrun'])
        self.assertDictContainsSubset(
                {'verify': False, 'update': False, 'actions': False,
                 'report': False, 'dryrun': True},
                vars(results))


if __name__ == '__main__':
    unittest.main()
