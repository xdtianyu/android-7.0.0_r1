#!/usr/bin/python
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
# pylint: disable-msg=C0111

import os, unittest
import mox
import common
import subprocess
import types
from autotest_lib.server import utils
from autotest_lib.server.cros.dynamic_suite import constants
from autotest_lib.site_utils import test_runner_utils


class StartsWithList(mox.Comparator):
    def __init__(self, start_of_list):
        """Mox comparator which returns True if the argument
        to the mocked function is a list that begins with the elements
        in start_of_list.
        """
        self._lhs = start_of_list

    def equals(self, rhs):
        if len(rhs)<len(self._lhs):
            return False
        for (x, y) in zip(self._lhs, rhs):
            if x != y:
                return False
        return True


class ContainsSublist(mox.Comparator):
    def __init__(self, sublist):
        """Mox comparator which returns True if the argument
        to the mocked function is a list that contains sublist
        as a sub-list.
        """
        self._sublist = sublist

    def equals(self, rhs):
        n = len(self._sublist)
        if len(rhs)<n:
            return False
        return any((self._sublist == rhs[i:i+n])
                   for i in xrange(len(rhs) - n + 1))


class TestRunnerUnittests(unittest.TestCase):

    def test_fetch_local_suite(self):
        # Deferred until fetch_local_suite knows about non-local builds.
        pass

    def test_get_predicate_for_test_arg(self):
        # Assert the type signature of get_predicate_for_test(...)
        # Because control.test_utils_wrapper calls this function,
        # it is imperative for backwards compatilbility that
        # the return type of the tested function does not change.
        tests = ['dummy_test', 'e:name_expression', 'f:expression',
                 'suite:suitename']
        for test in tests:
            pred, desc = test_runner_utils.get_predicate_for_test_arg(test)
            self.assertTrue(isinstance(pred, types.FunctionType))
            self.assertTrue(isinstance(desc, str))

    def test_run_job(self):
        class Object():
            pass

        autotest_path = 'htap_tsetotua'
        autoserv_command = os.path.join(autotest_path, 'server', 'autoserv')
        remote = 'etomer'
        results_dir = '/tmp/fakeresults'
        fast_mode = False
        job1_results_dir = '/tmp/fakeresults/results-1-gilbert'
        job2_results_dir = '/tmp/fakeresults/results-2-sullivan'
        args = 'matey'
        expected_args_sublist = ['--args', args]
        experimental_keyval = {constants.JOB_EXPERIMENTAL_KEY: False}
        self.mox = mox.Mox()

        # Create some dummy job objects.
        job1 = Object()
        job2 = Object()
        setattr(job1, 'control_type', 'cLiEnT')
        setattr(job1, 'control_file', 'c1')
        setattr(job1, 'id', 1)
        setattr(job1, 'name', 'gilbert')
        setattr(job1, 'keyvals', experimental_keyval)

        setattr(job2, 'control_type', 'Server')
        setattr(job2, 'control_file', 'c2')
        setattr(job2, 'id', 2)
        setattr(job2, 'name', 'sullivan')
        setattr(job2, 'keyvals', experimental_keyval)

        id_digits = 1

        # Stub out subprocess.Popen and wait calls.
        # Make them expect correct arguments.
        def fake_readline():
            return b''
        mock_process_1 = self.mox.CreateMock(subprocess.Popen)
        mock_process_2 = self.mox.CreateMock(subprocess.Popen)
        fake_stdout = self.mox.CreateMock(file)
        fake_returncode = 0
        mock_process_1.stdout = fake_stdout
        mock_process_1.returncode = fake_returncode
        mock_process_2.stdout = fake_stdout
        mock_process_2.returncode = fake_returncode

        self.mox.StubOutWithMock(os, 'makedirs')
        self.mox.StubOutWithMock(utils, 'write_keyval')
        self.mox.StubOutWithMock(subprocess, 'Popen')

        os.makedirs(job1_results_dir)
        utils.write_keyval(job1_results_dir, experimental_keyval)
        arglist_1 = [autoserv_command, '-p', '-r', job1_results_dir,
                     '-m', remote, '--no_console_prefix', '-l', 'gilbert',
                     '-c']
        subprocess.Popen(mox.And(StartsWithList(arglist_1),
                                 ContainsSublist(expected_args_sublist)),
                         stdout=subprocess.PIPE,
                         stderr=subprocess.STDOUT
                        ).AndReturn(mock_process_1)
        mock_process_1.stdout.readline().AndReturn(b'')
        mock_process_1.wait().AndReturn(0)

        os.makedirs(job2_results_dir)
        utils.write_keyval(job2_results_dir, experimental_keyval)
        arglist_2 = [autoserv_command, '-p', '-r', job2_results_dir,
                     '-m', remote,  '--no_console_prefix', '-l', 'sullivan',
                     '-s']
        subprocess.Popen(mox.And(StartsWithList(arglist_2),
                                 ContainsSublist(expected_args_sublist)),
                         stdout=subprocess.PIPE,
                         stderr=subprocess.STDOUT
                        ).AndReturn(mock_process_2)
        mock_process_2.stdout.readline().AndReturn(b'')
        mock_process_2.wait().AndReturn(0)

        # Test run_job.
        self.mox.ReplayAll()
        code, job_res = test_runner_utils.run_job(
                job1, remote, autotest_path,results_dir, fast_mode, id_digits,
                0, None, args)
        self.assertEqual(job_res, job1_results_dir)
        self.assertEqual(code, 0)
        code, job_res = test_runner_utils.run_job(
                job2, remote, autotest_path, results_dir, fast_mode, id_digits,
                0, None, args)

        self.assertEqual(job_res, job2_results_dir)
        self.assertEqual(code, 0)
        self.mox.UnsetStubs()
        self.mox.VerifyAll()
        self.mox.ResetAll()

    def test_perform_local_run(self):
        afe = test_runner_utils.setup_local_afe()
        autotest_path = 'ottotest_path'
        suite_name = 'sweet_name'
        test_arg = 'suite:' + suite_name
        remote = 'remoat'
        build = 'bild'
        board = 'bored'
        fast_mode = False
        suite_control_files = ['c1', 'c2', 'c3', 'c4']
        results_dir = '/tmp/test_that_results_fake'
        id_digits = 1
        ssh_verbosity = 2
        ssh_options = '-F /dev/null -i /dev/null'
        args = 'matey'
        ignore_deps = False

        # Fake suite objects that will be returned by fetch_local_suite
        class fake_suite(object):
            def __init__(self, suite_control_files, hosts):
                self._suite_control_files = suite_control_files
                self._hosts = hosts

            def schedule(self, *args, **kwargs):
                for control_file in self._suite_control_files:
                    afe.create_job(control_file, hosts=self._hosts)

        # Mock out scheduling of suite and running of jobs.
        self.mox = mox.Mox()

        self.mox.StubOutWithMock(test_runner_utils, 'fetch_local_suite')
        test_runner_utils.fetch_local_suite(autotest_path, mox.IgnoreArg(),
                afe, test_arg=test_arg, remote=remote, build=build,
                board=board, results_directory=results_dir,
                no_experimental=False,
                ignore_deps=ignore_deps
                ).AndReturn(fake_suite(suite_control_files, [remote]))
        self.mox.StubOutWithMock(test_runner_utils, 'run_job')
        self.mox.StubOutWithMock(test_runner_utils, 'run_provisioning_job')
        self.mox.StubOutWithMock(test_runner_utils, '_auto_detect_labels')

        test_runner_utils._auto_detect_labels(afe, remote)
        # Test perform_local_run. Enforce that run_provisioning_job,
        # run_job and _auto_detect_labels are called correctly.
        test_runner_utils.run_provisioning_job(
                'cros-version:' + build, remote, autotest_path,
                 results_dir, fast_mode,
                 ssh_verbosity, ssh_options,
                 False, False)

        for control_file in suite_control_files:
            test_runner_utils.run_job(
                    mox.ContainsAttributeValue('control_file', control_file),
                    remote, autotest_path, results_dir, fast_mode,id_digits,
                    ssh_verbosity, ssh_options,args, False,
                    False, {}).AndReturn((0, '/fake/dir'))
        self.mox.ReplayAll()
        test_runner_utils.perform_local_run(
                afe, autotest_path, ['suite:'+suite_name], remote, fast_mode,
                build=build, board=board, ignore_deps=False,
                ssh_verbosity=ssh_verbosity, ssh_options=ssh_options,
                args=args, results_directory=results_dir)
        self.mox.UnsetStubs()
        self.mox.VerifyAll()


if __name__ == '__main__':
    unittest.main()
