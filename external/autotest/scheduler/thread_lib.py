# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


"""Thread library for drone management.

This library contains a threaded task queue capable of starting, monitoring
and syncing threads across remote and localhost drones asynchronously. It also
contains a wrapper for standard python threads that records exceptions so they
can be re-raised in the thread manager. The api exposed by the threaded task
queue is as follows:
    1. worker: The staticmethod executed by all worker threads.
    2. execute: Takes a list of drones and invokes a worker thread per drone.
        This method assumes that all drones have a queue of pending calls
        for execution.
    3. wait_on_drones: Waits for all worker threads started by execute to finish
        and raises any exceptions as a consolidated DroneTaskQueueException.
    4. get_results: Returns the results of all threads as a dictionary keyed
        on the drones.
"""

import collections
import Queue
import threading
import logging

import common
from autotest_lib.client.common_lib.cros.graphite import autotest_stats
from autotest_lib.scheduler import drone_task_queue


class ExceptionRememberingThread(threading.Thread):
    """A wrapper around regular python threads that records exceptions."""

    def run(self):
        """Wrapper around the thread's run method."""
        try:
            with autotest_stats.Timer(self.name):
                super(ExceptionRememberingThread, self).run()
        except Exception as self.err:
            logging.error('%s raised an exception that will be re-raised by '
                          'the thread pool manager.', self.getName())
        else:
            self.err = None


class PersistentTimer(object):
    """A class to handle timers across local scopes."""

    def __init__(self, name):
        """Initialize a persistent timer.

        @param name: The name/key to insert timings under.
        """
        self.name = name
        self.timer = None


    def start(self):
        """Create and start a new timer."""
        self.timer = autotest_stats.Timer(self.name)
        self.timer.start()


    def stop(self):
        """Stop a previously started timer."""
        try:
            self.timer.stop()
        except (AssertionError, AttributeError) as e:
            logging.info('Stopping timer %s failed: %s', self.name, e)
        finally:
            self.timer = None


class ThreadedTaskQueue(drone_task_queue.DroneTaskQueue):
    """Threaded implementation of a drone task queue."""

    result = collections.namedtuple('task', ['drone', 'results'])

    def __init__(self, name='thread_queue'):
        self.results_queue = Queue.Queue()
        self.drone_threads = {}
        self.name = name
        # The persistent timer is used to measure net time spent
        # refreshing all drones across 'execute' and 'get_results'.
        self.timer = PersistentTimer(self.name)


    @staticmethod
    def worker(drone, results_queue):
        """Worker for task execution.

        Execute calls queued against the given drone and place the return value
        in results_queue.

        @param drone: A drone with calls to execute.
        @param results_queue: A queue, into which the worker places
            ThreadedTaskQueue.result from the drone calls.
        """
        logging.info('(Worker.%s) starting.', drone.hostname)
        results_queue.put(ThreadedTaskQueue.result(
            drone, drone.execute_queued_calls()))
        logging.info('(Worker.%s) finished.', drone.hostname)


    def wait_on_drones(self):
        """Wait on all threads that are currently refreshing a drone.

        @raises DroneTaskQueueException: Consolidated exception for all
            drone thread exceptions.
        """
        if not self.drone_threads:
            return
        # TODO: Make this process more resilient. We can:
        # 1. Timeout the join.
        # 2. Kick out the exception/timeout drone.
        # 3. Selectively retry exceptions.
        # For now, it is compliant with the single threaded drone manager which
        # will raise all drone_utility, ssh and drone_manager exceptions.
        drone_exceptions = []
        for drone, thread in self.drone_threads.iteritems():
            tname = thread.getName()
            logging.info('(Task Queue) Waiting for %s', tname)
            thread.join()
            if thread.err:
                drone_exceptions.append((drone, thread.err))
        logging.info('(Task Queue) All threads have returned, clearing map.')
        self.drone_threads = {}
        if not drone_exceptions:
            return
        exception_msg = ''
        for drone, err in drone_exceptions:
            exception_msg += ('Drone %s raised Exception %s\n' %
                              (drone.hostname, err))
        raise drone_task_queue.DroneTaskQueueException(exception_msg)


    def get_results(self):
        """Get a results dictionary keyed on the drones.

        This method synchronously waits till all drone threads have returned
        before checking for results. It is meant to be invoked in conjunction
        with the 'execute' method, which creates a thread per drone.

        @return: A dictionary of return values from the drones.
        """
        self.wait_on_drones()
        self.timer.stop()
        results = {}
        while not self.results_queue.empty():
            drone_results = self.results_queue.get()
            if drone_results.drone in results:
                raise drone_task_queue.DroneTaskQueueException(
                        'Task queue has recorded results for drone %s: %s' %
                        (drone_results.drone, results))
            results[drone_results.drone] = drone_results.results
        return results


    def execute(self, drones, wait=True):
        """Invoke a thread per drone, to execute drone_utility in parallel.

        @param drones: A list of drones with calls to execute.
        @param wait: If True, this method will only return when all the drones
            have returned the result of their respective invocations of
            drone_utility. The results_queue and drone_threads will be cleared.
            If False, the caller must clear both the queue and the map before
            the next invocation of 'execute', by calling 'get_results'.

        @return: A dictionary keyed on the drones, containing a list of return
            values from the execution of drone_utility.

        @raises DroneManagerError: If the results queue or drone map isn't empty
            at the time of invocation.
        """
        if not self.results_queue.empty():
            raise drone_task_queue.DroneTaskQueueException(
                    'Cannot clobber results queue: %s, it should be cleared '
                    'through get_results.' % self.results_queue)
        if self.drone_threads:
            raise drone_task_queue.DroneTaskQueueException(
                    'Cannot clobber thread map: %s, it should be cleared '
                    'through wait_on_drones' % self.drone_threads)
        self.timer.start()
        for drone in drones:
            if not drone.get_calls():
                continue
            worker_thread = ExceptionRememberingThread(
                    target=ThreadedTaskQueue.worker,
                    args=(drone, self.results_queue))
            # None of these threads are allowed to survive past the tick they
            # were spawned in, and the scheduler won't die mid-tick, so none
            # of the threads need to be daemons. However, if the scheduler does
            # die unexpectedly we can just forsake the daemon threads.
            self.drone_threads[drone] = worker_thread
            # The name is only used for debugging
            worker_thread.setName('%s.%s' %
                                  (self.name, drone.hostname.replace('.', '_')))
            worker_thread.daemon = True
            worker_thread.start()
        return self.get_results() if wait else None
