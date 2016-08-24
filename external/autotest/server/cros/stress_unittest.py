# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import threading
import unittest

import stress


class StopThreadForTesting(Exception):
    pass


class StressorTest(unittest.TestCase):

    def testEscalateExceptions(self):
        def stress_event():
            raise StopThreadForTesting

        stressor = stress.CountedStressor(stress_event)
        stressor.start(1)
        self.assertRaises(StopThreadForTesting, stressor.wait)


    def testDontEscalateExceptions(self):
        event = threading.Event()
        def stress_event():
            event.set()
            raise StopThreadForTesting

        stressor = stress.CountedStressor(stress_event,
                                          escalate_exceptions=False)
        stressor.start(1)
        stressor.wait()
        self.assertTrue(event.is_set(), 'The stress event did not run')


    def testOnExit(self):
        def stress_event():
            pass

        event = threading.Event()
        def on_exit():
            event.set()

        stressor = stress.CountedStressor(stress_event, on_exit=on_exit)
        stressor.start(1)
        stressor.wait()
        self.assertTrue(event.is_set())


    def testOnExitWithException(self):
        def stress_event():
            raise StopThreadForTesting

        event = threading.Event()
        def on_exit():
            event.set()

        stressor = stress.CountedStressor(stress_event, on_exit=on_exit)
        stressor.start(1)
        self.assertRaises(StopThreadForTesting, stressor.wait)
        self.assertTrue(event.is_set())


    def testCountedStressorStartCondition(self):
        event = threading.Event()

        def start_condition():
            if event.is_set():
                return True
            event.set()
            return False

        def stress_event():
            raise StopThreadForTesting

        stressor = stress.CountedStressor(stress_event)
        stressor.start(1, start_condition=start_condition)
        self.assertRaises(StopThreadForTesting, stressor.wait)
        self.assertTrue(event.is_set(),
                        'Stress event ran despite a False start condition')


    def testControlledStressorStartCondition(self):
        start_event = threading.Event()
        ran_event = threading.Event()

        def start_condition():
            if start_event.is_set():
                return True
            start_event.set()
            return False

        def stress_event():
            ran_event.set()
            raise StopThreadForTesting

        stressor = stress.ControlledStressor(stress_event)
        stressor.start(start_condition=start_condition)
        ran_event.wait()
        self.assertRaises(StopThreadForTesting, stressor.stop)
        self.assertTrue(start_event.is_set(),
                        'Stress event ran despite a False start condition')


    def testCountedStressorIterations(self):
        # This is a list to get around scoping rules in Python 2.x. See
        # 'nonlocal' for the Python 3 remedy.
        count = [0]

        def stress_event():
            count[0] += 1

        stressor = stress.CountedStressor(stress_event)
        stressor.start(10)
        stressor.wait()
        self.assertEqual(10, count[0], 'Stress event did not run the expected '
                                       'number of iterations')


if __name__ == '__main__':
    unittest.main()
