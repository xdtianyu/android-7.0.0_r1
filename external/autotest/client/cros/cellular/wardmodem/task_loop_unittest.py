#!/usr/bin/env python

# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import task_loop

import glib
import logging
import mox
import time
import unittest

class TaskLoopTestCase(unittest.TestCase):
    """
    Test fixture for TaskLoop class.

    These unit-tests have a trade-off between speed and stability. The whole
    suite takes about half a minute to run, and probably could be speeded up a
    little bit. But, making the numbers too small might make the tests flaky.
    """

    def setUp(self):
        self._mox = mox.Mox()
        self._callback_mocker = self._mox.CreateMock(TaskLoopTestCase)
        self._task_loop = task_loop.get_instance()

    # ##########################################################################
    # Tests

    def test_post_task_simple(self):
        """Post a simple task and expect it gets dispatched."""
        self._callback_mocker._callback()

        self._mox.ReplayAll()
        self._task_loop.post_task(self._callback_mocker._callback)
        self._run_task_loop(2)
        self._mox.VerifyAll()


    def test_post_task_set_attribute(self):
        """Post a task that accesses an attribute from the context object."""
        self.flag = False
        self._task_loop.post_task(self._callback_set_attribute)
        self._run_task_loop(2)
        self.assertTrue(self.flag)


    def test_post_task_with_argument(self):
        """Post task with some argument."""
        arg = True
        self._callback_mocker._callback_with_arguments(arg)

        self._mox.ReplayAll()
        self._task_loop.post_task(
                self._callback_mocker._callback_with_arguments, arg)
        self._run_task_loop(2)
        self._mox.VerifyAll()


    def test_post_task_after_delay(self):
        """Post a task with some delay and check that the delay is respected."""
        start_time = time.time()
        self.time = start_time
        self._task_loop.post_task_after_delay(self._callback_set_time, 3000)
        self._run_task_loop(5)
        delayed_time = self.time - start_time
        self.assertGreaterEqual(delayed_time, 3)


    def test_post_repeated_task(self):
        """Post a repeated task and check it gets dispatched multiple times."""
        self.count = 0
        self._task_loop.post_repeated_task(self._callback_increment_count, 1000)
        self._run_task_loop(5)
        self.assertGreaterEqual(self.count, 3)


    def test_ignore_delays(self):
        """Post a task and test ignore_delays mode."""
        self._task_loop.ignore_delays = False

        self._task_loop.post_task_after_delay(self._callback_mocker._callback,
                                              10000)
        # Not enough time to dispatch the posted task
        self._run_task_loop(1)
        self._mox.VerifyAll()


    def test_cancel_posted_task(self):
        """Test that a cancelled task is not dispatched."""
        post_id = self._task_loop.post_task_after_delay(
                self._callback_mocker._callback,
                2000)
        self._task_loop.post_task(self._callback_cancel_task, post_id)
        self._run_task_loop(3)
        self._mox.VerifyAll()


    def test_multiple_cancels(self):
        """Test that successive cancels after a successful cancel fail."""
        post_id = self._task_loop.post_task_after_delay(
                self._callback_mocker._callback,
                2000)
        self._task_loop.post_task(self._callback_cancel_task, post_id)
        self._task_loop.post_task(self._callback_cancel_cancelled_task, post_id)
        self._run_task_loop(3)
        self._mox.VerifyAll()


    def test_random_delays(self):
        """Test that random delays works (sort of). This test could be flaky."""
        # Warning: This test could be flaky. Add more differences?
        self.count = 0
        self.times = {}
        self._task_loop.random_delays = True
        self._task_loop.max_random_delay_ms = 1000
        self._task_loop.post_repeated_task(self._callback_record_times, 500)
        self._run_task_loop(5)
        self.assertGreaterEqual(self.count, 4)
        # Test that not all time gaps are almost the same
        diff1 = round(self.times[1] - self.times[0], 3)
        diff2 = round(self.times[2] - self.times[1], 3)
        diff3 = round(self.times[3] - self.times[2], 3)
        self.assertTrue(diff1 != diff2 or diff2 != diff3 or diff3 != diff1)

    # ##########################################################################
    # Helper functions

    def _stop_task_loop(self):
        print('Stopping task_loop.')
        self._task_loop.stop()

    def _run_task_loop(self, run_for_seconds):
        """
        Runs the task loop for |run_for_seconds| seconds. This function is
        blocking, so the main thread will return only after |run_for_seconds|.
        """
        # post a task to stop the task loop.
        glib.timeout_add(run_for_seconds*1000, self._stop_task_loop)
        self._task_loop.start()
        # We will continue only when the stop task has been executed.

    # ##########################################################################
    # Callbacks for tests

    def _callback(self):
        print('Actual TaskLoopTestCase._callback called!')


    def _callback_set_attribute(self):
        self.flag = True


    def _callback_set_time(self):
        self.time = time.time()


    def _callback_increment_count(self):
        self.count = self.count + 1


    def _callback_record_times(self):
        self.times[self.count] = time.time()
        self.count = self.count + 1


    def _callback_with_arguments(self, arg):
        pass


    def _callback_cancel_task(self, post_id):
        self._task_loop.cancel_posted_task(post_id)


    def _callback_cancel_cancelled_task(self, post_id):
        self.assertFalse(self._task_loop.cancel_posted_task(post_id))


if __name__ == '__main__':
    logging.basicConfig(level=logging.DEBUG)
    unittest.main()
