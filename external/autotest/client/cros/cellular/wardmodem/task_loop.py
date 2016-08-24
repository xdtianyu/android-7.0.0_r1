#!/usr/bin/env python

# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import glib
import logging
import random

DEFAULT_MAX_RANDOM_DELAY_MS = 10000

_instance = None

def get_instance():
    """
    Return the singleton instance of the TaskLoop class.

    """
    global _instance
    if _instance is None:
        _instance = TaskLoop()
    return _instance


class TaskLoop(object):
    """
    The context to place asynchronous calls.

    This is a wrapper around the GLIB mainloop interface, exposing methods to
    place (delayed) asynchronous calls. In addition to wrapping around the GLIB
    API, this provides switches to control how delays are incorporated in method
    calls globally.

    This class is meant to be a singleton.
    Do not create an instance directly, use the module level function
    get_instance() instead.

    Running the TaskLoop is blocking for the caller. So use this class like so:

    tl = task_loop.get_instance()
    # Setup other things.
    # Add initial tasks to tl to do stuff, post more tasks, and make the world a
    # better place.
    tl.start()
    # This thread is now blocked. Some task should eventually call tl.stop() to
    continue here.

    @var ignore_delays: Flag to control if delayed tasks are posted immediately.

    @var random_delays: Flag to control if arbitrary delays are inserted between
            posted tasks.

    @var max_random_delay_ms: When random_delays is True, the maximum delay
            inserted between posted tasks.

    """


    def __init__(self):
        self._logger = logging.getLogger(__name__)

        # Initialize properties
        self._ignore_delays = False
        self._random_delays = False
        self._max_random_delay_ms = DEFAULT_MAX_RANDOM_DELAY_MS

        # Get the mainloop so that tasks can be posted even before running the
        # task loop.
        self._mainloop = glib.MainLoop()

        # Initialize dictionary to track posted tasks.
        self._next_post_id = 0
        self._posted_tasks = {}


    @property
    def ignore_delays(self):
        """
        Boolean flag to control if delayed tasks are posted immediately.

        If True, all tasks posted henceforth are immediately marked active
        ignoring any delay requested. With this switch, all other delay related
        switches are ignored.

        """
        return self._ignore_delays


    @ignore_delays.setter
    def ignore_delays(self, value):
        """
        Set |ignore_delays|.

        @param value: Boolean value for the |ignore_delays| flag

        """
        self._logger.debug('Turning %s delays ignored mode.', ('on' if value
                           else 'off'))
        self._ignore_delays = value


    @property
    def random_delays(self):
        """
        Boolean flag to control if random delays are inserted in posted tasks.

        If True, arbitrary delays in range [0, |max_random_delay_ms|] are
        inserted in all posted tasks henceforth, ignoring the actual delay
        requested.

        """
        return self._random_delays


    @random_delays.setter
    def random_delays(self, value):
        """
        Set |random_delays|.

        @param value: Boolean value for the random_delays flag.

        """
        self._logger.debug('Turning %s random delays.', ('on' if value else
                                                         'off'))
        self._random_delays = value


    @property
    def max_random_delay_ms(self):
        """
        The maximum arbitrary delay inserted in posted tasks in milliseconds.
        Type: int

        """
        return self._max_random_delay_ms


    @max_random_delay_ms.setter
    def max_random_delay_ms(self, value):
        """
        Set |max_random_delay_ms|.

        @param value: Non-negative int value for |max_random_delay_ms|. Negative
                values are clamped to 0.

        """
        if value < 0:
            self._logger.warning(
                    'Can not set max_random_delay_ms to negative value %s. '
                    'Setting to 0 instead.',
                    value)
            value = 0
        self._logger.debug('Set max random delay to %d. Random delay is %s',
                           value, ('on' if self.random_delays else 'off'))
        self._max_random_delay_ms = value


    def start(self):
        """
        Run the task loop.

        This call is blocking. The thread that calls TaskLoop.start(...) becomes
        the task loop itself and is blocked as such till TaskLoop.stop(...) is
        called.

        """
        self._logger.info('Task Loop is now processing tasks...')
        self._mainloop.run()


    def stop(self):
        """
        Stop the task loop.

        """
        self._logger.info('Task Loop quitting.')
        self._mainloop.quit()


    def post_repeated_task(self, callback, delay_ms=0):
        """
        Post the given callback repeatedly forever until cancelled.

        The posted callback must not expect any arguments. It likely does not
        make sense to provide fixed data parameters to a repeated task. Use the
        object reference to provide context.

        In the |ignore_delays| mode, the task is reposted immediately after
        dispatch.
        In the |random_delays| mode, a new arbitrary delay is inserted before
        each call to |callback|.

        @param callback: The function to call repeatedly. |callback| must expect
                an object reference as the only argument. The return value from
                |callback| is ignored.

        @param delay_ms: The delay between repeated calls to |callback|. The
                first call is also delayed by this amount. Default: 0

        @return: An integer ID that can be used to cancel the posted task.

        """
        assert callback is not None

        post_id = self._next_post_id
        self._next_post_id += 1

        next_delay_ms = self._next_delay_ms(delay_ms)
        self._posted_tasks[post_id]  = glib.timeout_add(
                next_delay_ms,
                TaskLoop._execute_repeated_task,
                self,
                post_id,
                callback,
                delay_ms)
        return post_id


    def post_task_after_delay(self, callback, delay_ms, *args, **kwargs):
        """
        Post the given callback once to be dispatched after |delay_ms|.

        @param callback: The function to call. The function may expect arbitrary
                number of arguments, passed in as |*args| and |**kwargs|. The
                return value from |callback| is ignored.

        @param delay_ms: The delay before the call to |callback|. Default: 0

        @return: An integer ID that can be used to cancel the posted task.

        """
        assert callback is not None
        post_id = self._next_post_id
        self._next_post_id = self._next_post_id + 1
        delay_ms = self._next_delay_ms(delay_ms)
        self._posted_tasks[post_id] = glib.timeout_add(delay_ms, callback,
                                                       *args, **kwargs)
        return post_id


    def post_task(self, callback, *args, **kwargs):
        """
        Post the given callback once.

        In |random_delays| mode, this function is equivalent to
        |post_task_after_delay|.

        @param callback: The function to call. The function may expect arbitrary
                number of arguments, passed in as |*args| and |**kwargs|. The
                return value from |callback| is ignored.

        @return: An integer ID that can be used to cancel the posted task.

        """
        self._logger.debug('Task posted: %s', repr(callback))
        self._logger.debug('Arguments: %s, Keyword arguments: %s',
                           repr(args), repr(kwargs))
        return self.post_task_after_delay(callback, 0, *args, **kwargs)


    def cancel_posted_task(self, post_id):
        """
        Cancels a previously posted task that is yet to be dispatched.

        @param post_id: The |post_id| of the task to cancel, as returned by one
                of the functions that post a task.

        @return: True if the posted task was removed.

        """
        if post_id in self._posted_tasks:
            retval = glib.source_remove(self._posted_tasks[post_id])
            if retval:
                del self._posted_tasks[post_id]
            return retval
        else:
            return False


    def _next_delay_ms(self, user_delay_ms):
        """
        Determine the actual delay to post the next task.

        The actual delay posted may be different from the user requested delay
        based on what mode we're in.

        @param user_delay_ms: The delay requested by the user.

        @return The actual delay to be posted.

        """
        next_delay_ms = user_delay_ms
        if self.ignore_delays:
            next_delay_ms = 0
        elif self.random_delays:
            next_delay_ms = random.randint(0, self.max_random_delay_ms)
        return next_delay_ms


    def _execute_repeated_task(self, post_id, callback, delay_ms):
        """
        A wrapper to repost an executed task, and return False.

        We need this to be able to repost the task at arbitrary intervals.

        @param post_id: The private post_id tracking this repeated task.

        @param callback: The user callback that must be called.

        @param delay_ms: The user requested delay between calls.

        """
        retval = callback()
        self._logger.debug('Ignored return value from repeated task: %s',
                           repr(retval))

        next_delay_ms = self._next_delay_ms(delay_ms)
        self._posted_tasks[post_id]  = glib.timeout_add(
                next_delay_ms,
                TaskLoop._execute_repeated_task,
                self,
                post_id,
                callback,
                delay_ms)
        return False
