#!/usr/bin/python
#
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unit tests for site_utils/task.py."""

import mox, unittest

# driver must be imported first due to circular imports in base_event and task
import driver  # pylint: disable-msg=W0611
import deduping_scheduler, forgiving_config_parser, task, build_event


class TaskTestBase(mox.MoxTestBase):
    """Common code for Task test classes

    @var _BUILD: fake build.
    @var _BOARD: fake board to reimage.
    @var _BRANCH: fake branch to run tests on.
    @var _BRANCH_SPEC: fake branch specification for Tasks.
    @var _MAP: fake branch:build map.
    @var _POOL: fake pool of machines to test on.
    @var _SUITE: fake suite name.
    @var _TASK_NAME: fake name for tasks in config.
    """

    _BUILD = 'build'
    _BOARD = 'board1'
    _BRANCH = '20'
    _BRANCH_SPEC = '>=R' + _BRANCH
    _BRANCH_SPEC_EQUAL = '==R' + _BRANCH
    _BRANCH_SPEC_LTE = '<=R' + _BRANCH
    _MAP = {_BRANCH: [_BUILD]}
    _NUM = 2
    _POOL = 'fake_pool'
    _SUITE = 'suite'
    _TASK_NAME = 'fake_task_name'
    _PRIORITY = build_event.BuildEvent.PRIORITY
    _TIMEOUT = build_event.BuildEvent.TIMEOUT
    _FILE_BUGS=False


    def setUp(self):
        super(TaskTestBase, self).setUp()
        self.sched = self.mox.CreateMock(deduping_scheduler.DedupingScheduler)


class TaskCreateTest(TaskTestBase):
    """Unit tests for Task.CreateFromConfigSection().

    @var _EVENT_KEY: fake event-to-run-on keyword for tasks in config.
    """

    _EVENT_KEY = 'new_build'


    def setUp(self):
        super(TaskCreateTest, self).setUp()
        self.config = forgiving_config_parser.ForgivingConfigParser()
        self.config.add_section(self._TASK_NAME)
        self.config.set(self._TASK_NAME, 'suite', self._SUITE)
        self.config.set(self._TASK_NAME, 'branch_specs', self._BRANCH_SPEC)
        self.config.set(self._TASK_NAME, 'run_on', self._EVENT_KEY)
        self.config.set(self._TASK_NAME, 'pool', self._POOL)
        self.config.set(self._TASK_NAME, 'num', '%d' % self._NUM)
        self.config.set(self._TASK_NAME, 'boards', self._BOARD)


    def testCreateFromConfig(self):
        """Ensure a Task can be built from a correct config."""
        keyword, new_task = task.Task.CreateFromConfigSection(self.config,
                                                              self._TASK_NAME)
        self.assertEquals(keyword, self._EVENT_KEY)
        self.assertEquals(new_task, task.Task(self._TASK_NAME, self._SUITE,
                                              [self._BRANCH_SPEC], self._POOL,
                                              self._NUM, self._BOARD,
                                              self._PRIORITY, self._TIMEOUT))
        self.assertTrue(new_task._FitsSpec(self._BRANCH))
        self.assertFalse(new_task._FitsSpec('12'))


    def testCreateFromConfigEqualBranch(self):
        """Ensure a Task can be built from a correct config with support of
        branch_specs: ==RXX."""
        # Modify the branch_specs setting in self.config.
        self.config.set(self._TASK_NAME, 'branch_specs',
                        self._BRANCH_SPEC_EQUAL)
        keyword, new_task = task.Task.CreateFromConfigSection(self.config,
                                                              self._TASK_NAME)
        self.assertEquals(keyword, self._EVENT_KEY)
        self.assertEquals(new_task, task.Task(self._TASK_NAME, self._SUITE,
                                              [self._BRANCH_SPEC_EQUAL],
                                              self._POOL, self._NUM,
                                              self._BOARD, self._PRIORITY,
                                              self._TIMEOUT))
        self.assertTrue(new_task._FitsSpec(self._BRANCH))
        self.assertFalse(new_task._FitsSpec('12'))
        self.assertFalse(new_task._FitsSpec('21'))
        # Reset the branch_specs setting in self.config to >=R.
        self.config.set(self._TASK_NAME, 'branch_specs', self._BRANCH_SPEC)


    def testCreateFromConfigLessThanOrEqualBranch(self):
        """Ensure a Task can be built from a correct config with support of
        branch_specs: <=RXX."""
        # Modify the branch_specs setting in self.config.
        self.config.set(self._TASK_NAME, 'branch_specs',
                        self._BRANCH_SPEC_LTE)
        keyword, new_task = task.Task.CreateFromConfigSection(self.config,
                                                              self._TASK_NAME)
        self.assertEquals(keyword, self._EVENT_KEY)
        self.assertEquals(new_task, task.Task(self._TASK_NAME, self._SUITE,
                                              [self._BRANCH_SPEC_LTE],
                                              self._POOL, self._NUM,
                                              self._BOARD, self._PRIORITY,
                                              self._TIMEOUT))
        self.assertTrue(new_task._FitsSpec(self._BRANCH))
        self.assertTrue(new_task._FitsSpec('12'))
        self.assertFalse(new_task._FitsSpec('21'))
        # Reset the branch_specs setting in self.config to >=R.
        self.config.set(self._TASK_NAME, 'branch_specs', self._BRANCH_SPEC)


    def testCreateFromConfigNoBranch(self):
        """Ensure a Task can be built from a correct config with no branch."""
        self.config.remove_option(self._TASK_NAME, 'branch_specs')
        keyword, new_task = task.Task.CreateFromConfigSection(self.config,
                                                              self._TASK_NAME)
        self.assertEquals(keyword, self._EVENT_KEY)
        self.assertEquals(new_task, task.Task(self._TASK_NAME, self._SUITE,
                                              [], self._POOL, self._NUM,
                                              self._BOARD, self._PRIORITY,
                                              self._TIMEOUT))
        self.assertTrue(new_task._FitsSpec(self._BRANCH))


    def testCreateFromConfigMultibranch(self):
        """Ensure a Task can be built from a correct config with >1 branches."""
        specs = ['factory', self._BRANCH_SPEC]
        self.config.set(self._TASK_NAME, 'branch_specs', ','.join(specs))
        keyword, new_task = task.Task.CreateFromConfigSection(self.config,
                                                              self._TASK_NAME)
        self.assertEquals(keyword, self._EVENT_KEY)
        self.assertEquals(new_task, task.Task(self._TASK_NAME, self._SUITE,
                                              specs, self._POOL, self._NUM,
                                              self._BOARD, self._PRIORITY,
                                              self._TIMEOUT))
        for spec in [specs[0], self._BRANCH]:
            self.assertTrue(new_task._FitsSpec(spec))


    def testCreateFromConfigNoNum(self):
        """Ensure a Task can be built from a correct config with no num."""
        self.config.remove_option(self._TASK_NAME, 'num')
        keyword, new_task = task.Task.CreateFromConfigSection(self.config,
                                                              self._TASK_NAME)
        self.assertEquals(keyword, self._EVENT_KEY)
        self.assertEquals(new_task, task.Task(self._TASK_NAME, self._SUITE,
                                              [self._BRANCH_SPEC], self._POOL,
                                              boards=self._BOARD))
        self.assertTrue(new_task._FitsSpec(self._BRANCH))
        self.assertFalse(new_task._FitsSpec('12'))


    def testCreateFromNoSuiteConfig(self):
        """Ensure we require a suite in Task config."""
        self.config.remove_option(self._TASK_NAME, 'suite')
        self.assertRaises(task.MalformedConfigEntry,
                          task.Task.CreateFromConfigSection,
                          self.config,
                          self._TASK_NAME)


    def testCreateFromNoKeywordConfig(self):
        """Ensure we require a run_on event in Task config."""
        self.config.remove_option(self._TASK_NAME, 'run_on')
        self.assertRaises(task.MalformedConfigEntry,
                          task.Task.CreateFromConfigSection,
                          self.config,
                          self._TASK_NAME)


    def testCreateFromNonexistentConfig(self):
        """Ensure we fail gracefully if we pass in a bad section name."""
        self.assertRaises(task.MalformedConfigEntry,
                          task.Task.CreateFromConfigSection,
                          self.config,
                          'not_a_thing')


    def testFileBugsNoConfigValue(self):
        """Ensure not setting file bugs in a config leads to file_bugs=False."""
        keyword, new_task = task.Task.CreateFromConfigSection(self.config,
                                                              self._TASK_NAME)
        self.assertFalse(new_task._file_bugs)


class TaskTest(TaskTestBase):
    """Unit tests for Task."""


    def setUp(self):
        super(TaskTest, self).setUp()
        self.task = task.Task(self._TASK_NAME, self._SUITE, [self._BRANCH_SPEC],
                              None, None, self._BOARD, self._PRIORITY,
                              self._TIMEOUT)


    def testRun(self):
        """Test running a recurring task."""
        self.sched.ScheduleSuite(self._SUITE, self._BOARD, self._BUILD,
                                 None, None, self._PRIORITY, self._TIMEOUT,
                                 False, file_bugs=self._FILE_BUGS,
                                 firmware_rw_build=None,
                                 test_source_build=None,
                                 job_retry=False).AndReturn(True)
        self.mox.ReplayAll()
        self.assertTrue(self.task.Run(self.sched, self._MAP, self._BOARD))


    def testRunCustomSharding(self):
        """Test running a recurring task with non-default sharding."""
        expected_sharding = 2
        mytask = task.Task(self._TASK_NAME, self._SUITE, [self._BRANCH_SPEC],
                           num=expected_sharding)
        self.sched.ScheduleSuite(self._SUITE, self._BOARD, self._BUILD,
                                 None, expected_sharding, None, None,
                                 False, file_bugs=self._FILE_BUGS,
                                 firmware_rw_build=None,
                                 test_source_build=None,
                                 job_retry=False).AndReturn(True)
        self.mox.ReplayAll()
        self.assertTrue(mytask.Run(self.sched, self._MAP, self._BOARD))


    def testRunDuplicate(self):
        """Test running a task that schedules a duplicate suite task."""
        self.sched.ScheduleSuite(self._SUITE, self._BOARD, self._BUILD,
                                 None, None, self._PRIORITY, self._TIMEOUT,
                                 False, file_bugs=self._FILE_BUGS,
                                 firmware_rw_build=None,
                                 test_source_build=None,
                                 job_retry=False).AndReturn(True)
        self.mox.ReplayAll()
        self.assertTrue(self.task.Run(self.sched, self._MAP, self._BOARD))


    def testRunUnrunnablePool(self):
        """Test running a task that cannot run on this pool."""
        self.sched.CheckHostsExist(
                multiple_labels=mox.IgnoreArg()).AndReturn(None)
        self.mox.ReplayAll()
        t = task.Task(self._TASK_NAME, self._SUITE,
                      [self._BRANCH_SPEC], "BadPool")
        self.assertTrue(not t.AvailableHosts(self.sched, self._BOARD))


    def testRunUnrunnableBoard(self):
        """Test running a task that cannot run on this board."""
        self.mox.ReplayAll()
        t = task.Task(self._TASK_NAME, self._SUITE,
                      [self._BRANCH_SPEC], self._POOL, boards="BadBoard")
        self.assertTrue(not t.AvailableHosts(self.sched, self._BOARD))


    def testNoRunBranchMismatch(self):
        """Test running a recurring task with no matching builds."""
        t = task.Task(self._TASK_NAME, self._SUITE, task.BARE_BRANCHES)
        self.mox.ReplayAll()
        self.assertTrue(t.Run(self.sched, self._MAP, self._BOARD))


    def testNoRunBareBranchMismatch(self):
        """Test running a recurring task with no matching builds (factory)."""
        self.mox.ReplayAll()
        self.assertTrue(
            self.task.Run(self.sched, {'factory': 'build2'}, self._BOARD))


    def testRunNoSpec(self):
        """Test running a recurring task with default branch specs."""
        t = task.Task(self._TASK_NAME, self._SUITE, [])
        self.sched.ScheduleSuite(self._SUITE, self._BOARD, self._BUILD,
                                 None, None, None, None,
                                 False, file_bugs=self._FILE_BUGS,
                                 firmware_rw_build=None,
                                 test_source_build=None,
                                 job_retry=False).AndReturn(True)
        self.mox.ReplayAll()
        self.assertTrue(t.Run(self.sched, self._MAP, self._BOARD))


    def testRunExplodes(self):
        """Test a failure to schedule while running task."""
        # Barf while scheduling.
        self.sched.ScheduleSuite(
            self._SUITE, self._BOARD, self._BUILD, None, None, self._PRIORITY,
            self._TIMEOUT, False, file_bugs=self._FILE_BUGS,
            firmware_rw_build=None, test_source_build=None,
            job_retry=False).AndRaise(
                    deduping_scheduler.ScheduleException('Simulated Failure'))
        self.mox.ReplayAll()
        self.assertTrue(self.task.Run(self.sched, self._MAP, self._BOARD))


    def testForceRun(self):
        """Test force running a recurring task."""
        self.sched.ScheduleSuite(self._SUITE, self._BOARD, self._BUILD,
                                 None, None, self._PRIORITY, self._TIMEOUT,
                                 True, file_bugs=self._FILE_BUGS,
                                 firmware_rw_build=None,
                                 test_source_build=None,
                                 job_retry=False).AndReturn(True)
        self.mox.ReplayAll()
        self.assertTrue(self.task.Run(self.sched, self._MAP, self._BOARD, True))


    def testHash(self):
        """Test hash function for Task classes."""
        same_task = task.Task(self._TASK_NAME, self._SUITE, [self._BRANCH_SPEC],
                              boards=self._BOARD)
        other_task = task.Task(self._TASK_NAME, self._SUITE,
                               [self._BRANCH_SPEC, '>=RX1'], 'pool')
        self.assertEquals(hash(self.task), hash(same_task))
        self.assertNotEquals(hash(self.task), hash(other_task))


class OneShotTaskTest(TaskTestBase):
    """Unit tests for OneShotTask."""


    def setUp(self):
        super(OneShotTaskTest, self).setUp()
        self.task = task.OneShotTask(self._TASK_NAME, self._SUITE,
                                     [self._BRANCH_SPEC])


    def testRun(self):
        """Test running a one-shot task."""
        self.sched.ScheduleSuite(self._SUITE, self._BOARD, self._BUILD,
                                 None, None, None, None, False,
                                 file_bugs=self._FILE_BUGS,
                                 firmware_rw_build=None,
                                 test_source_build=None,
                                 job_retry=False).AndReturn(True)
        self.mox.ReplayAll()
        self.assertFalse(self.task.Run(self.sched, self._MAP, self._BOARD))


    def testRunDuplicate(self):
        """Test running a one-shot task that schedules a dup suite task."""
        self.sched.ScheduleSuite(self._SUITE, self._BOARD, self._BUILD,
                                 None, None, None, None, False,
                                 file_bugs=self._FILE_BUGS,
                                 firmware_rw_build=None,
                                 test_source_build=None,
                                 job_retry=False).AndReturn(False)
        self.mox.ReplayAll()
        self.assertFalse(self.task.Run(self.sched, self._MAP, self._BOARD))


    def testRunExplodes(self):
        """Test a failure to schedule while running one-shot task."""
        # Barf while scheduling.
        self.sched.ScheduleSuite(
            self._SUITE, self._BOARD, self._BUILD, None, None,
            None, None, False, file_bugs=self._FILE_BUGS,
            firmware_rw_build=None, test_source_build=None,
            job_retry=False).AndRaise(
                deduping_scheduler.ScheduleException('Simulated Failure'))
        self.mox.ReplayAll()
        self.assertFalse(self.task.Run(self.sched, self._MAP, self._BOARD))


    def testForceRun(self):
        """Test force running a one-shot task."""
        self.sched.ScheduleSuite(self._SUITE, self._BOARD, self._BUILD,
                                 None, None, None, None, True,
                                 file_bugs=self._FILE_BUGS,
                                 firmware_rw_build=None,
                                 test_source_build=None,
                                 job_retry=False).AndReturn(True)
        self.mox.ReplayAll()
        self.assertFalse(self.task.Run(self.sched, self._MAP, self._BOARD,
                                       force=True))


    def testFileBugs(self):
        """Test that file_bugs is passed from the task to ScheduleSuite."""
        self.sched.ScheduleSuite(self._SUITE, self._BOARD, self._BUILD,
                                 None, None, None, None, True,
                                 file_bugs=True, firmware_rw_build=None,
                                 test_source_build=None,
                                 job_retry=False).AndReturn(True)
        self.mox.ReplayAll()
        self.task._file_bugs = True
        self.assertFalse(self.task.Run(self.sched, self._MAP, self._BOARD,
                                       force=True))


if __name__ == '__main__':
    unittest.main()
