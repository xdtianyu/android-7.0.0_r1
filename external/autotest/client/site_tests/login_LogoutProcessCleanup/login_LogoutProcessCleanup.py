# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, signal, utils

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import chrome


class _TestProcess:


    def __init__(self, command, pattern):
        self.command = command
        self.pattern = pattern
        self.pid_su = ''
        self.pid_bash = ''


    def __wait_for_subprocess(self):
        """Waits for a subprocess that matches self.pattern."""
        def _subprocess_pid(pattern):
            pid = utils.system_output('ps -U chronos -o pid,args | grep %s'
                                      % pattern, ignore_status=True)
            return pid.lstrip().split(' ')[0] if pid else 0

        utils.poll_for_condition(lambda: _subprocess_pid(self.pattern))
        self.pid_bash = _subprocess_pid(self.pattern)


    def run_me_as_chronos(self):
        """Runs the command in self.command as user 'chronos'.

        Waits for bash sub-process to start, and fails if this does not happen.

        """
        # Start process as user chronos.
        self.pid_su = utils.BgJob('su chronos -c "%s"' % self.command)
        # Get pid of bash sub-process. Even though utils.BgJob() has exited,
        # the su-process may not have created its sub-process yet.
        self.__wait_for_subprocess()
        return self.pid_bash != ''


class login_LogoutProcessCleanup(test.test):
    """Tests that all processes owned by chronos are destroyed on logout."""
    version = 1


    def __get_session_manager_pid(self):
        """Get the PID of the session manager."""
        return utils.system_output('pgrep "^session_manager$"',
                                   ignore_status=True)


    def __get_chronos_pids(self):
        """Get a list of all PIDs that are owned by chronos."""
        return utils.system_output('pgrep -U chronos',
                                   ignore_status=True).splitlines()


    def __get_stat_fields(self, pid):
        """Get a list of strings for the fields in /proc/pid/stat.

        @param pid: process to stat.
        """
        with open('/proc/%s/stat' % pid) as stat_file:
            return stat_file.read().split(' ')


    def __get_parent_pid(self, pid):
        """Get the parent PID of the given process.

        @param pid: process whose parent pid you want to look up.
        """
        return self.__get_stat_fields(pid)[3]


    def __is_process_dead(self, pid):
        """Check whether or not a process is dead.  Zombies are dead.

        @param pid: process to check on.
        """
        try:
            if self.__get_stat_fields(pid)[2] == 'Z':
                return True
        except IOError:
            # If the proc entry is gone, it's dead.
            return True
        return False


    def __process_has_ancestor(self, pid, ancestor_pid):
        """Tests if pid has ancestor_pid anywhere in the process tree.

        @param pid: pid whose ancestry the caller is searching.
        @param ancestor_pid: the ancestor to look for.
        """
        ppid = pid
        while not (ppid == ancestor_pid or ppid == '0'):
            # This could fail if the process is killed while we are
            # looking up the parent.  In that case, treat it as if it
            # did not have the ancestor.
            try:
                ppid = self.__get_parent_pid(ppid)
            except IOError:
                return False
        return ppid == ancestor_pid


    def __has_chronos_processes(self, session_manager_pid):
        """Looks for chronos processes not started by the session manager.

        @param session_manager_pid: pid of the session_manager.
        """
        pids = self.__get_chronos_pids()
        for p in pids:
            if self.__is_process_dead(p):
                continue
            if not self.__process_has_ancestor(p, session_manager_pid):
                logging.info('Found pid (%s) owned by chronos and not '
                             'started by the session manager.', p)
                return True
        return False


    def run_once(self):
        with chrome.Chrome() as cr:
            test_processes = []
            test_processes.append(
                    _TestProcess('while :; do :; done ; # tst00','bash.*tst00'))
            # Create a test command that ignores SIGTERM.
            test_processes.append(
                    _TestProcess('trap 15; while :; do :; done ; # tst01',
                                 'bash.*tst01'))

            for test in test_processes:
                if not test.run_me_as_chronos():
                    raise error.TestFail(
                            'Did not start: bash %s' % test.command)

            session_manager = self.__get_session_manager_pid()
            if not session_manager:
                raise error.TestError('Could not find session manager pid')

            if not self.__has_chronos_processes(session_manager):
                raise error.TestFail(
                        'Expected to find processes owned by chronos that were '
                        'not started by the session manager while logged in.')

            cpids = self.__get_chronos_pids()

            # Sanity checks: make sure test jobs are in the list and still
            # running.
            for test in test_processes:
                if cpids.count(test.pid_bash) != 1:
                    raise error.TestFail('Job missing (%s - %s)' %
                                         (test.pid_bash, test.command))
                if self.__is_process_dead(test.pid_bash):
                    raise error.TestFail('Job prematurely dead (%s - %s)' %
                                         (test.pid_bash, test.command))

        logging.info('Logged out, searching for processes that should be dead.')

        # Wait until we have a new session manager.  At that point, all
        # old processes should be dead.
        old_session_manager = session_manager
        utils.poll_for_condition(
                lambda: old_session_manager != self.__get_session_manager_pid())
        session_manager = self.__get_session_manager_pid()

        # Make sure all pre-logout chronos processes are now dead.
        old_pid_count = 0
        for p in cpids:
            if not self.__is_process_dead(p):
                old_pid_count += 1
                proc_args = utils.system_output('ps -p %s -o args=' % p,
                                                ignore_status=True)
                logging.info('Found pre-logout chronos process pid=%s (%s) '
                             'still alive.', p, proc_args)
                # If p is something we started, kill it.
                for test in test_processes:
                    if (p == test.pid_su or p == test.pid_bash):
                        utils.signal_pid(p, signal.SIGKILL)

        if old_pid_count > 0:
            raise error.TestFail('Found %s chronos processes that survived '
                                 'logout.' % old_pid_count)
