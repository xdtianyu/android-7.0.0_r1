#!/usr/bin/python
#pylint: disable-msg=C0111

"""Tests for autotest_lib.scheduler.drones."""

import cPickle

import common
from autotest_lib.client.common_lib import utils
from autotest_lib.client.common_lib.test_utils import mock, unittest
from autotest_lib.scheduler import drones
from autotest_lib.server.hosts import ssh_host


class RemoteDroneTest(unittest.TestCase):

    def setUp(self):
        self.god = mock.mock_god()
        self._mock_host = self.god.create_mock_class(ssh_host.SSHHost,
                                                     'mock SSHHost')
        self.god.stub_function(drones.drone_utility, 'create_host')
        self.drone_utility_path = 'mock-drone-utility-path'


    def tearDown(self):
        self.god.unstub_all()


    def test_unreachable(self):
        drones.drone_utility.create_host.expect_call('fakehost').and_return(
                self._mock_host)
        self._mock_host.is_up.expect_call().and_return(False)
        self.assertRaises(drones.DroneUnreachable,
                          drones._RemoteDrone, 'fakehost')


    def test_execute_calls_impl(self):
        self.god.stub_with(drones._RemoteDrone, '_drone_utility_path',
                           self.drone_utility_path)
        drones.drone_utility.create_host.expect_call('fakehost').and_return(
                self._mock_host)
        self._mock_host.is_up.expect_call().and_return(True)
        mock_calls = ('foo',)
        mock_result = utils.CmdResult(stdout=cPickle.dumps('mock return'))
        self._mock_host.run.expect_call(
                'python %s' % self.drone_utility_path,
                stdin=cPickle.dumps(mock_calls), stdout_tee=None,
                connect_timeout=mock.is_instance_comparator(int)).and_return(
                        mock_result)
        drone = drones._RemoteDrone('fakehost', timestamp_remote_calls=False)
        self.assertEqual('mock return', drone._execute_calls_impl(mock_calls))
        self.god.check_playback()


    def test_execute_queued_calls(self):
        self.god.stub_with(drones._RemoteDrone, '_drone_utility_path',
                           self.drone_utility_path)
        drones.drone_utility.create_host.expect_call('fakehost').and_return(
                self._mock_host)
        self._mock_host.is_up.expect_call().and_return(True)
        drone = drones._RemoteDrone('fakehost', timestamp_remote_calls=False)
        mock_return={}
        mock_return['results'] = ['mock return']
        mock_return['warnings'] = []
        drone.queue_call('foo')
        mock_result = utils.CmdResult(stdout=cPickle.dumps(mock_return))
        self._mock_host.run.expect_call(
                'python %s' % self.drone_utility_path,
                stdin=cPickle.dumps(drone.get_calls()), stdout_tee=None,
                connect_timeout=mock.is_instance_comparator(int)).and_return(
                        mock_result)
        self.assertEqual(mock_return['results'], drone.execute_queued_calls())
        self.god.check_playback()



if __name__ == '__main__':
    unittest.main()
