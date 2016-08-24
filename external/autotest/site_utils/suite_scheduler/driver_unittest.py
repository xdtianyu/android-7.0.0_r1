#!/usr/bin/python
#
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unit tests for site_utils/board_enumerator.py."""

import mox, unittest

# driver must be imported first due to circular imports in base_event and task
import driver  # pylint: disable-msg=W0611
import base_event, board_enumerator, build_event, deduping_scheduler
import forgiving_config_parser, manifest_versions, task, timed_event

import common
from autotest_lib.server import frontend
from constants import Labels


class DriverTest(mox.MoxTestBase):
    """Unit tests for Driver."""

    _BOARDS = ['board1', 'board2']


    def setUp(self):
        super(DriverTest, self).setUp()
        self.afe = self.mox.CreateMock(frontend.AFE)
        self.be = board_enumerator.BoardEnumerator(self.afe)
        self.ds = deduping_scheduler.DedupingScheduler(self.afe)
        self.mv = self.mox.CreateMock(manifest_versions.ManifestVersions)

        self.config = forgiving_config_parser.ForgivingConfigParser()

        self.nightly_bvt = task.Task(timed_event.Nightly.KEYWORD, '', '')
        self.weekly_bvt = task.Task(timed_event.Weekly.KEYWORD, '', '')
        self.new_build_bvt = task.Task(build_event.NewBuild.KEYWORD, '', '')

        self.driver = driver.Driver(self.ds, self.be)


    def _CreateMockEvent(self, klass):
        event = self.mox.CreateMock(klass)
        event.keyword = klass.KEYWORD
        event.tasks = []
        return event


    def _ExpectSetup(self):
        mock_nightly = self._CreateMockEvent(timed_event.Nightly)
        mock_weekly = self._CreateMockEvent(timed_event.Weekly)
        mock_new_build = self._CreateMockEvent(build_event.NewBuild)

        self.mox.StubOutWithMock(timed_event.Nightly, 'CreateFromConfig')
        self.mox.StubOutWithMock(timed_event.Weekly, 'CreateFromConfig')
        self.mox.StubOutWithMock(build_event.NewBuild, 'CreateFromConfig')
        timed_event.Nightly.CreateFromConfig(
            mox.IgnoreArg(), self.mv).AndReturn(mock_nightly)
        timed_event.Weekly.CreateFromConfig(
            mox.IgnoreArg(), self.mv).AndReturn(mock_weekly)
        build_event.NewBuild.CreateFromConfig(
            mox.IgnoreArg(), self.mv).AndReturn(mock_new_build)
        return [mock_nightly, mock_weekly, mock_new_build]


    def _ExpectTaskConfig(self):
        self.config.add_section(timed_event.Nightly.KEYWORD)
        self.config.add_section(timed_event.Weekly.KEYWORD)
        self.mox.StubOutWithMock(task.Task, 'CreateFromConfigSection')
        task.Task.CreateFromConfigSection(
            self.config, timed_event.Nightly.KEYWORD).InAnyOrder().AndReturn(
                (timed_event.Nightly.KEYWORD, self.nightly_bvt))
        task.Task.CreateFromConfigSection(
            self.config, timed_event.Weekly.KEYWORD).InAnyOrder().AndReturn(
                (timed_event.Weekly.KEYWORD, self.weekly_bvt))


    def _ExpectEnumeration(self):
        """Expect one call to BoardEnumerator.Enumerate()."""
        prefix = Labels.BOARD_PREFIX
        mocks = []
        for board in self._BOARDS:
            mocks.append(self.mox.CreateMock(frontend.Label))
            mocks[-1].name = prefix + board
        self.afe.get_labels(name__startswith=prefix).AndReturn(mocks)


    def _ExpectHandle(self, event, group):
        """Make event report that it's handle-able, and expect it to be handle.

        @param event: the mock event that expectations will be set on.
        @param group: group to put new expectations in.
        """
        bbs = {'branch': 'build-string'}
        event.ShouldHandle().InAnyOrder(group).AndReturn(True)
        for board in self._BOARDS:
            event.GetBranchBuildsForBoard(
                board).InAnyOrder(group).AndReturn(bbs)
            event.Handle(mox.IgnoreArg(), bbs, board).InAnyOrder(group)
        # Should happen once per loop, not once per Handle()
        # http://crosbug.com/30642
        event.UpdateCriteria().InAnyOrder(group)


    def _ExpectNoHandle(self, event, group):
        """Make event report that it's handle-able, but do not expect to
        handle it.

        @param event: the mock event that expectations will be set on.
        @param group: group to put new expectations in.
        """
        bbs = {'branch': 'build-string'}
        event.ShouldHandle().InAnyOrder(group).AndReturn(True)


    def testTasksFromConfig(self):
        """Test that we can build a list of Tasks from a config."""
        self._ExpectTaskConfig()
        self.mox.ReplayAll()
        tasks = self.driver.TasksFromConfig(self.config)
        self.assertTrue(self.nightly_bvt in tasks[timed_event.Nightly.KEYWORD])
        self.assertTrue(self.weekly_bvt in tasks[timed_event.Weekly.KEYWORD])


    def testTasksFromConfigRecall(self):
        """Test that we can build a list of Tasks from a config twice."""
        events = self._ExpectSetup()
        self._ExpectTaskConfig()
        self.mox.ReplayAll()

        self.driver.SetUpEventsAndTasks(self.config, self.mv)
        for keyword, event in self.driver._events.iteritems():
            if keyword == timed_event.Nightly.KEYWORD:
                self.assertTrue(self.nightly_bvt in event.tasks)
            if keyword == timed_event.Weekly.KEYWORD:
                self.assertTrue(self.weekly_bvt in event.tasks)

        self.mox.UnsetStubs()
        self.mox.VerifyAll()

        self.mox.ResetAll()
        self.config.remove_section(timed_event.Weekly.KEYWORD)

        self._ExpectSetup()
        self.mox.StubOutWithMock(task.Task, 'CreateFromConfigSection')
        task.Task.CreateFromConfigSection(
            self.config, timed_event.Nightly.KEYWORD).InAnyOrder().AndReturn(
                (timed_event.Nightly.KEYWORD, self.nightly_bvt))
        self.mox.ReplayAll()
        self.driver.SetUpEventsAndTasks(self.config, self.mv)
        for keyword, event in self.driver._events.iteritems():
            if keyword == timed_event.Nightly.KEYWORD:
                self.assertTrue(self.nightly_bvt in event.tasks)
            elif keyword == timed_event.Weekly.KEYWORD:
                self.assertFalse(self.weekly_bvt in event.tasks)


    def testHandleAllEventsOnce(self):
        """Test that all events being ready is handled correctly."""
        events = self._ExpectSetup()
        self._ExpectEnumeration()
        for event in events:
            self._ExpectHandle(event, 'events')
        self.mox.ReplayAll()

        driver.POOL_SIZE = 1
        self.driver.SetUpEventsAndTasks(self.config, self.mv)
        self.driver.HandleEventsOnce(self.mv)


    def testHandleNightlyEventOnce(self):
        """Test that one ready event is handled correctly."""
        events = self._ExpectSetup()
        self._ExpectEnumeration()
        for event in events:
            if event.keyword == timed_event.Nightly.KEYWORD:
                self._ExpectHandle(event, 'events')
            else:
                event.ShouldHandle().InAnyOrder('events').AndReturn(False)
        self.mox.ReplayAll()

        driver.POOL_SIZE = 1
        self.driver.SetUpEventsAndTasks(self.config, self.mv)
        self.driver.HandleEventsOnce(self.mv)


    def testForceOnceForBuild(self):
        """Test that one event being forced is handled correctly."""
        events = self._ExpectSetup()

        board = 'board'
        type = 'release'
        milestone = '00'
        manifest = '200.0.02'
        build = base_event.BuildName(board, type, milestone, manifest)

        events[0].Handle(mox.IgnoreArg(), {milestone: [build]}, board,
                            force=True)
        self.mox.ReplayAll()

        self.driver.SetUpEventsAndTasks(self.config, self.mv)
        self.driver.ForceEventsOnceForBuild([events[0].keyword], build)


if __name__ == '__main__':
    unittest.main()
