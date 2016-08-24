#!/usr/bin/python

import common
from autotest_lib.frontend import setup_django_environment
from autotest_lib.frontend.afe import frontend_test_utils
from autotest_lib.client.common_lib.test_utils import unittest
from autotest_lib.frontend.afe import models
from autotest_lib.scheduler import agent_task
from autotest_lib.server import system_utils


class RestrictedSubnetTest(unittest.TestCase,
                           frontend_test_utils.FrontendTestMixin):
    """Test server election based on restricted subnet setting.
    """

    DRONE_IN_RESTRICTED_SUBNET = '192.168.0.9'
    DRONE_NOT_IN_RESTRICTED_SUBNET = '127.0.0.9'
    HOST_IN_RESTRICTED_SUBNET = '192.168.0.3'
    HOST_NOT_IN_RESTRICTED_SUBNET = '127.0.0.3'
    RESTRICTED_SUBNETS = [('192.168.0.1', 16)]

    def setUp(self):
        self._drones = [self.DRONE_IN_RESTRICTED_SUBNET,
                        self.DRONE_NOT_IN_RESTRICTED_SUBNET]
        system_utils.DroneCache.unrestricted_drones = None
        system_utils.DroneCache.drone_ip_map = None
        self._frontend_common_setup()


    def tearDown(self):
        self._frontend_common_teardown()


    def test_get_drone_hostnames_allowed_with_restricted_subnet(self):
        """Test method get_drone_hostnames_allowed work as expected when
        restricted subnet is set, and host is inside restricted subnet.
        """
        self.god.stub_function(system_utils, 'get_drones')
        system_utils.get_drones.expect_call().and_return(self._drones)
        self.god.stub_function(models.DroneSet, 'drone_sets_enabled')
        models.DroneSet.drone_sets_enabled.expect_call().and_return(False)

        task = agent_task.AgentTask()
        task.hostnames = {1: self.HOST_IN_RESTRICTED_SUBNET}
        self.assertEqual(
                set([self.DRONE_IN_RESTRICTED_SUBNET]),
                task.get_drone_hostnames_allowed(self.RESTRICTED_SUBNETS, True))
        self.god.check_playback()


    def test_get_drone_hostnames_allowed_not_in_restricted_subnet(self):
        """Test method get_drone_hostnames_allowed work as expected when
        restricted subnet is set, and host is not in restricted subnet.
        """
        self.god.stub_function(system_utils, 'get_drones')
        system_utils.get_drones.expect_call().and_return(self._drones)
        self.god.stub_function(models.DroneSet, 'drone_sets_enabled')
        models.DroneSet.drone_sets_enabled.expect_call().and_return(False)

        task = agent_task.AgentTask()
        task.hostnames = {1: self.HOST_NOT_IN_RESTRICTED_SUBNET}
        self.assertEqual(
                set([self.DRONE_NOT_IN_RESTRICTED_SUBNET]),
                task.get_drone_hostnames_allowed(self.RESTRICTED_SUBNETS, True))
        self.god.check_playback()


    def test_get_drone_hostnames_allowed_in_mixed_subnet(self):
        """Test method get_drone_hostnames_allowed work as expected when
        restricted subnet is set, and hosts are distributed across restricted
        subnet and unrestricted subnet.
        """
        task = agent_task.AgentTask()
        task.hostnames = {1: self.HOST_NOT_IN_RESTRICTED_SUBNET,
                          2: self.HOST_IN_RESTRICTED_SUBNET}
        self.assertEqual(
                set(),
                task.get_drone_hostnames_allowed(self.RESTRICTED_SUBNETS, True))
        self.god.check_playback()


if __name__ == '__main__':
    unittest.main()
