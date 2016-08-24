# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import sys
import threading
import time
from autotest_lib.client.common_lib import error


class BaseStressor(threading.Thread):
    """
    Implements common functionality for *Stressor classes.

    @var stressor: callable which performs a single stress event.
    """
    def __init__(self, stressor, on_exit=None, escalate_exceptions=True):
        """
        Initialize the ControlledStressor.

        @param stressor: callable which performs a single stress event.
        @param on_exit: callable which will be called when the thread finishes.
        @param escalate_exceptions: whether to escalate exceptions to the parent
            thread; defaults to True.
        """
        super(BaseStressor, self).__init__()
        self.daemon = True
        self.stressor = stressor
        self.on_exit = on_exit
        self._escalate_exceptions = escalate_exceptions
        self._exc_info = None


    def start(self, start_condition=None, start_timeout_secs=None):
        """
        Creates a new thread which will call the run() method.

        Optionally takes a wait condition before the stressor loop. Returns
        immediately.

        @param start_condition: the new thread will wait until this optional
            callable returns True before running the stressor.
        @param start_timeout_secs: how long to wait for |start_condition| to
            become True, or None to wait forever.
        """
        self._start_condition = start_condition
        self._start_timeout_secs = start_timeout_secs
        super(BaseStressor, self).start()


    def run(self):
        """
        Wait for |_start_condition|, and then start the stressor loop.

        Overloaded from threading.Thread. This is run in a separate thread when
        start() is called.
        """
        try:
            self._wait_for_start_condition()
            self._loop_stressor()
        except Exception as e:
            if self._escalate_exceptions:
                self._exc_info = sys.exc_info()
            raise  # Terminates this thread. Caller continues to run.
        finally:
            if self.on_exit:
              self.on_exit()


    def _wait_for_start_condition(self):
        """
        Loop until _start_condition() returns True, or _start_timeout_secs
        have elapsed.

        @raise error.TestFail if we time out waiting for the start condition
        """
        if self._start_condition is None:
            return

        elapsed_secs = 0
        while not self._start_condition():
            if (self._start_timeout_secs and
                    elapsed_secs >= self._start_timeout_secs):
                raise error.TestFail('start condition did not become true '
                                     'within %d seconds' %
                                     self._start_timeout_secs)
            time.sleep(1)
            elapsed_secs += 1


    def _loop_stressor(self):
        """
        Apply stressor in a loop.

        Overloaded by the particular *Stressor.
        """
        raise NotImplementedError


    def reraise(self):
        """
        Reraise an exception raised in the thread's stress loop.

        This is a No-op if no exception was raised.
        """
        if self._exc_info:
            exc_info = self._exc_info
            self._exc_info = None
            raise exc_info[0], exc_info[1], exc_info[2]


class ControlledStressor(BaseStressor):
    """
    Run a stressor in loop on a separate thread.

    Creates a new thread and calls |stressor| in a loop until stop() is called.
    """
    def __init__(self, stressor, on_exit=None, escalate_exceptions=True):
        """
        Initialize the ControlledStressor.

        @param stressor: callable which performs a single stress event.
        @param on_exit: callable which will be called when the thread finishes.
        @param escalate_exceptions: whether to escalate exceptions to the parent
            thread when stop() is called; defaults to True.
        """
        self._complete = threading.Event()
        super(ControlledStressor, self).__init__(stressor, on_exit,
                                                 escalate_exceptions)


    def _loop_stressor(self):
        """Overloaded from parent."""
        iteration_num = 0
        while not self._complete.is_set():
            iteration_num += 1
            logging.info('Stressor iteration: %d', iteration_num)
            self.stressor()


    def start(self, start_condition=None, start_timeout_secs=None):
        """Start applying the stressor.

        Overloaded from parent.

        @param start_condition: the new thread will wait to until this optional
            callable returns True before running the stressor.
        @param start_timeout_secs: how long to wait for |start_condition| to
            become True, or None to wait forever.
        """
        self._complete.clear()
        super(ControlledStressor, self).start(start_condition,
                                              start_timeout_secs)


    def stop(self, timeout=45):
        """
        Stop applying the stressor.

        @param timeout: maximum time to wait for a single run of the stressor to
            complete, defaults to 45 seconds.
        """
        self._complete.set()
        self.join(timeout)
        self.reraise()


class CountedStressor(BaseStressor):
    """
    Run a stressor in a loop on a separate thread a given number of times.

    Creates a new thread and calls |stressor| in a loop |iterations| times. The
    calling thread can use wait() to block until the loop completes. If the
    stressor thread terminates with an exception, wait() will propagate that
    exception to the thread that called wait().
    """
    def _loop_stressor(self):
        """Overloaded from parent."""
        for iteration_num in xrange(1, self._iterations + 1):
            logging.info('Stressor iteration: %d of %d',
                         iteration_num, self._iterations)
            self.stressor()


    def start(self, iterations, start_condition=None, start_timeout_secs=None):
        """
        Apply the stressor a given number of times.

        Overloaded from parent.

        @param iterations: number of times to apply the stressor.
        @param start_condition: the new thread will wait to until this optional
            callable returns True before running the stressor.
        @param start_timeout_secs: how long to wait for |start_condition| to
            become True, or None to wait forever.
        """
        self._iterations = iterations
        super(CountedStressor, self).start(start_condition, start_timeout_secs)


    def wait(self, timeout=None):
        """Wait until the stressor completes.

        @param timeout: maximum time for the thread to complete, by default
            never times out.
        """
        self.join(timeout)
        self.reraise()
