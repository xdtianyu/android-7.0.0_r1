#!/usr/bin/python
#
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Tests for the drone task queue."""

import cPickle
import logging
import Queue
import unittest

import common
from autotest_lib.client.common_lib import utils
from autotest_lib.client.common_lib.test_utils import mock, unittest
from autotest_lib.scheduler import drone_task_queue
from autotest_lib.scheduler import drones
from autotest_lib.server.hosts import ssh_host


class DroneTaskQueueTest(unittest.TestCase):
    """Drone task queue tests."""

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


    def test_execute(self):
        """Test execute method."""

        drone1 = self.create_remote_drone('fakehostname1')
        drone2 = self.create_remote_drone('fakehostname2')
        drone3 = self.create_remote_drone('fakehostname3')

        # Check task queue exception conditions.
        task_queue = drone_task_queue.DroneTaskQueue()
        task_queue.results[drone1] = None
        self.assertRaises(drone_task_queue.DroneTaskQueueException,
                          task_queue.execute, [])
        task_queue.results.clear()

        # Queue 2 calls against each drone, and confirm that the host's
        # run method is called 3 times. Then compare results returned by
        # the task queue against the mock results.
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
        self.assertTrue(set(task_queue.results.keys()) == set(drones))
        results = task_queue.get_results()
        self.assertTrue(len(task_queue.results) == 0)
        for drone, result in results.iteritems():
            self.assertTrue(result == self.mock_return['results'])

        # Test execute and get_results
        drone1.queue_call('foo')
        mock_result = utils.CmdResult(stdout=cPickle.dumps(self.mock_return))
        self._mock_host.run.expect_call(
                'python %s' % self.drone_utility_path,
                stdin=cPickle.dumps(drone1.get_calls()), stdout_tee=None,
                connect_timeout=mock.is_instance_comparator(int)).and_return(
                        mock_result)
        self.assertTrue(task_queue.execute(drones, wait=True)[drone1] ==
                        self.mock_return['results'])
        self.assertTrue(len(task_queue.results) == 0)
        self.assertTrue(len(task_queue.get_results()) == 0)
        self.god.check_playback()


if __name__ == '__main__':
    unittest.main()
