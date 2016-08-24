#!/usr/bin/python
#
# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Tests for the drone managers thread queue."""

import cPickle
import logging
import Queue
import unittest

import common
from autotest_lib.client.common_lib import utils
from autotest_lib.client.common_lib.test_utils import mock, unittest
from autotest_lib.scheduler import drone_task_queue
from autotest_lib.scheduler import drones
from autotest_lib.scheduler import thread_lib
from autotest_lib.server.hosts import ssh_host


class DroneThreadLibTest(unittest.TestCase):
    """Threaded task queue drone library tests."""

    def create_remote_drone(self, hostname):
        """Create and initialize a Remote Drone.

        @param hostname: The name of the host for the remote drone.

        @return: A remote drone instance.
        """
        drones.drone_utility.create_host.expect_call(hostname).and_return(
                self._mock_host)
        self._mock_host.is_up.expect_call().and_return(True)
        return drones._RemoteDrone(hostname, timestamp_remote_calls=False)


    def setUp(self):
        self.god = mock.mock_god()
        self._mock_host = self.god.create_mock_class(ssh_host.SSHHost,
                                                     'mock SSHHost')
        self.god.stub_function(drones.drone_utility, 'create_host')
        self.drone_utility_path = 'mock-drone-utility-path'
        self.mock_return = {'results': ['mock results'],
                            'warnings': []}
        self.god.stub_with(drones._RemoteDrone, '_drone_utility_path',
                self.drone_utility_path)


    def tearDown(self):
        self.god.unstub_all()


    def test_worker(self):
        """Test the worker method of a ThreadedTaskQueue."""
        # Invoke the worker method with a drone that has a queued call and check
        # that the drones host.run method is invoked for the call, and the
        # results queue contains the expected results.
        drone = self.create_remote_drone('fakehostname')
        task_queue = thread_lib.ThreadedTaskQueue()

        drone.queue_call('foo')
        mock_result = utils.CmdResult(stdout=cPickle.dumps(self.mock_return))
        self._mock_host.run.expect_call(
                'python %s' % self.drone_utility_path,
                stdin=cPickle.dumps(drone.get_calls()), stdout_tee=None,
                connect_timeout=mock.is_instance_comparator(int)).and_return(
                        mock_result)
        task_queue.worker(drone, task_queue.results_queue)
        result = task_queue.results_queue.get()

        self.assertTrue(task_queue.results_queue.empty() and
                        result.drone == drone and
                        result.results == self.mock_return['results'])
        self.god.check_playback()


    def test_wait_on_drones(self):
        """Test waiting on drone threads."""

        def waiting_func(queue):
            while len(queue.queue) < 2:
                continue
            logging.warning('Consuming thread finished.')
            queue.put(3)

        def exception_func(queue):
            while queue.empty():
                continue
            queue.put(2)
            logging.warning('Failing thread raising error.')
            raise ValueError('Value error')

        def quick_func():
            return

        # Create 2 threads, one of which raises an exception while the other
        # just exits normally. Insert both threads into the thread_queue against
        # mock drones and confirm that:
        # a. The task queue waits for both threads, though the first one fails.
        # b. The task queue records the right DroneTaskQueueException, which
        #       contains the original exception.
        # c. The failing thread records its own exception instead of raising it.
        task_queue = thread_lib.ThreadedTaskQueue()
        drone1 = self.create_remote_drone('fakehostname1')
        drone2 = self.create_remote_drone('fakehostname2')
        sync_queue = Queue.Queue()

        waiting_worker = thread_lib.ExceptionRememberingThread(
                target=waiting_func, args=(sync_queue,))
        failing_worker = thread_lib.ExceptionRememberingThread(
                target=exception_func, args=(sync_queue,))
        task_queue.drone_threads[drone1] = waiting_worker
        task_queue.drone_threads[drone2] = failing_worker
        master_thread = thread_lib.ExceptionRememberingThread(
                target=task_queue.wait_on_drones)

        thread_list = [failing_worker, waiting_worker, master_thread]
        for thread in thread_list:
            thread.setDaemon(True)
            thread.start()
        sync_queue.put(1)
        master_thread.join()

        self.assertTrue(isinstance(master_thread.err,
                                   drone_task_queue.DroneTaskQueueException))
        self.assertTrue(isinstance(failing_worker.err, ValueError))
        self.assertTrue(str(failing_worker.err) in str(master_thread.err))
        self.assertTrue(3 in list(sync_queue.queue))
        self.assertTrue(task_queue.drone_threads == {})

        # Call wait_on_drones after the child thread has exited.
        quick_worker = thread_lib.ExceptionRememberingThread(target=quick_func)
        task_queue.drone_threads[drone1] = quick_worker
        quick_worker.start()
        while quick_worker.isAlive():
            continue
        task_queue.wait_on_drones()
        self.assertTrue(task_queue.drone_threads == {})


    def test_get_results(self):
        """Test retrieving results from the results queue."""

        # Insert results for the same drone twice into the results queue
        # and confirm that an exception is raised.
        task_queue = thread_lib.ThreadedTaskQueue()
        drone1 = self.create_remote_drone('fakehostname1')
        drone2 = self.create_remote_drone('fakehostname2')
        task_queue.results_queue.put(
                thread_lib.ThreadedTaskQueue.result(drone1, self.mock_return))
        task_queue.results_queue.put(
                thread_lib.ThreadedTaskQueue.result(drone1, self.mock_return))
        self.god.stub_function(task_queue, 'wait_on_drones')
        task_queue.wait_on_drones.expect_call()
        self.assertRaises(drone_task_queue.DroneTaskQueueException,
                          task_queue.get_results)

        # Insert results for different drones and check that they're returned
        # in a drone results dict.
        self.assertTrue(task_queue.results_queue.empty())
        task_queue.results_queue.put(
                thread_lib.ThreadedTaskQueue.result(drone1, self.mock_return))
        task_queue.results_queue.put(
                thread_lib.ThreadedTaskQueue.result(drone2, self.mock_return))
        task_queue.wait_on_drones.expect_call()
        results = task_queue.get_results()
        self.assertTrue(results[drone1] == self.mock_return and
                        results[drone2] == self.mock_return)
        self.god.check_playback()


    def test_execute(self):
        """Test task queue execute."""
        drone1 = self.create_remote_drone('fakehostname1')
        drone2 = self.create_remote_drone('fakehostname2')
        drone3 = self.create_remote_drone('fakehostname3')

        # Check task queue exception conditions.
        task_queue = thread_lib.ThreadedTaskQueue()
        task_queue.results_queue.put(1)
        self.assertRaises(drone_task_queue.DroneTaskQueueException,
                          task_queue.execute, [])
        task_queue.results_queue.get()
        task_queue.drone_threads[drone1] = None
        self.assertRaises(drone_task_queue.DroneTaskQueueException,
                          task_queue.execute, [])
        task_queue.drone_threads = {}

        # Queue 2 calls against each drone, and confirm that the host's
        # run method is called 3 times. Then check the threads created,
        # and finally compare results returned by the task queue against
        # the mock results.
        drones = [drone1, drone2, drone3]
        for drone in drones:
            drone.queue_call('foo')
            drone.queue_call('bar')
            mock_result = utils.CmdResult(
                    stdout=cPickle.dumps(self.mock_return))
            self._mock_host.run.expect_call(
                    'python %s' % self.drone_utility_path,
                    stdin=cPickle.dumps(drone.get_calls()), stdout_tee=None,
                    connect_timeout=mock.is_instance_comparator(int)
                    ).and_return(mock_result)
        task_queue.execute(drones, wait=False)
        self.assertTrue(set(task_queue.drone_threads.keys()) == set(drones))
        for drone, thread in task_queue.drone_threads.iteritems():
            self.assertTrue(drone.hostname in thread.getName())
            self.assertTrue(thread.isDaemon())
            self.assertRaises(RuntimeError, thread.start)
        results = task_queue.get_results()
        for drone, result in results.iteritems():
            self.assertTrue(result == self.mock_return['results'])

        # Test synchronous execute
        drone1.queue_call('foo')
        mock_result = utils.CmdResult(stdout=cPickle.dumps(self.mock_return))
        self._mock_host.run.expect_call(
                'python %s' % self.drone_utility_path,
                stdin=cPickle.dumps(drone1.get_calls()), stdout_tee=None,
                connect_timeout=mock.is_instance_comparator(int)).and_return(
                        mock_result)
        self.assertTrue(task_queue.execute(drones, wait=True)[drone1] ==
                        self.mock_return['results'])
        self.god.check_playback()


if __name__ == '__main__':
    unittest.main()
