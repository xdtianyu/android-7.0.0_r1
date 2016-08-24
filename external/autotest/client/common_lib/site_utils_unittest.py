#!/usr/bin/python
#pylint: disable=C0111

import unittest

import common
from autotest_lib.client.common_lib import utils
from autotest_lib.client.common_lib.test_utils import mock


def test_function(arg1, arg2, arg3, arg4=4, arg5=5, arg6=6):
    """Test global function.
    """


class TestClass(object):
    """Test class.
    """

    def test_instance_function(self, arg1, arg2, arg3, arg4=4, arg5=5, arg6=6):
        """Test instance function.
        """


    @classmethod
    def test_class_function(cls, arg1, arg2, arg3, arg4=4, arg5=5, arg6=6):
        """Test class function.
        """


    @staticmethod
    def test_static_function(arg1, arg2, arg3, arg4=4, arg5=5, arg6=6):
        """Test static function.
        """


class GetFunctionArgUnittest(unittest.TestCase):
    """Tests for method get_function_arg_value."""

    def run_test(self, func, insert_arg):
        """Run test.

        @param func: Function being called with given arguments.
        @param insert_arg: Set to True to insert an object in the argument list.
                           This is to mock instance/class object.
        """
        if insert_arg:
            args = (None, 1, 2, 3)
        else:
            args = (1, 2, 3)
        for i in range(1, 7):
            self.assertEquals(utils.get_function_arg_value(
                    func, 'arg%d'%i, args, {}), i)

        self.assertEquals(utils.get_function_arg_value(
                func, 'arg7', args, {'arg7': 7}), 7)
        self.assertRaises(
                KeyError, utils.get_function_arg_value,
                func, 'arg3', args[:-1], {})


    def test_global_function(self):
        """Test global function.
        """
        self.run_test(test_function, False)


    def test_instance_function(self):
        """Test instance function.
        """
        self.run_test(TestClass().test_instance_function, True)


    def test_class_function(self):
        """Test class function.
        """
        self.run_test(TestClass.test_class_function, True)


    def test_static_function(self):
        """Test static function.
        """
        self.run_test(TestClass.test_static_function, False)


class VersionMatchUnittest(unittest.TestCase):
    """Test version_match function."""

    def test_version_match(self):
        """Test version_match function."""
        canary_build = 'lumpy-release/R43-6803.0.0'
        canary_release = '6803.0.0'
        cq_build = 'lumpy-release/R43-6803.0.0-rc1'
        cq_release = '6803.0.0-rc1'
        trybot_paladin_build = 'trybot-lumpy-paladin/R43-6803.0.0-b123'
        trybot_paladin_release = '6803.0.2015_03_12_2103'
        trybot_pre_cq_build = 'trybot-wifi-pre-cq/R43-7000.0.0-b36'
        trybot_pre_cq_release = '7000.0.2016_03_12_2103'


        builds = [canary_build, cq_build, trybot_paladin_build,
                  trybot_pre_cq_build]
        releases = [canary_release, cq_release, trybot_paladin_release,
                    trybot_pre_cq_release]
        for i in range(len(builds)):
            for j in range(len(releases)):
                self.assertEqual(
                        utils.version_match(builds[i], releases[j]), i==j,
                        'Build version %s should%s match release version %s.' %
                        (builds[i], '' if i==j else ' not', releases[j]))


class IsPuppylabVmUnittest(unittest.TestCase):
    """Test is_puppylab_vm function."""

    def test_is_puppylab_vm(self):
        """Test is_puppylab_vm function."""
        self.assertTrue(utils.is_puppylab_vm('localhost:8001'))
        self.assertTrue(utils.is_puppylab_vm('127.0.0.1:8002'))
        self.assertFalse(utils.is_puppylab_vm('localhost'))
        self.assertFalse(utils.is_puppylab_vm('localhost:'))
        self.assertFalse(utils.is_puppylab_vm('127.0.0.1'))
        self.assertFalse(utils.is_puppylab_vm('127.0.0.1:'))
        self.assertFalse(utils.is_puppylab_vm('chromeos-server.mtv'))
        self.assertFalse(utils.is_puppylab_vm('chromeos-server.mtv:8001'))


class IsInSameSubnetUnittest(unittest.TestCase):
    """Test is_in_same_subnet function."""

    def test_is_in_same_subnet(self):
        """Test is_in_same_subnet function."""
        self.assertTrue(utils.is_in_same_subnet('192.168.0.0', '192.168.1.2',
                                                23))
        self.assertFalse(utils.is_in_same_subnet('192.168.0.0', '192.168.1.2',
                                                24))
        self.assertTrue(utils.is_in_same_subnet('192.168.0.0', '192.168.0.255',
                                                24))
        self.assertFalse(utils.is_in_same_subnet('191.168.0.0', '192.168.0.0',
                                                24))

class GetWirelessSsidUnittest(unittest.TestCase):
    """Test get_wireless_ssid function."""

    DEFAULT_SSID = 'default'
    SSID_1 = 'ssid_1'
    SSID_2 = 'ssid_2'

    def test_get_wireless_ssid(self):
        """Test is_in_same_subnet function."""
        god = mock.mock_god()
        god.stub_function_to_return(utils.CONFIG, 'get_config_value',
                                    self.DEFAULT_SSID)
        god.stub_function_to_return(utils.CONFIG, 'get_config_value_regex',
                                    {'wireless_ssid_1.2.3.4/24': self.SSID_1,
                                     'wireless_ssid_4.3.2.1/16': self.SSID_2})
        self.assertEqual(self.SSID_1, utils.get_wireless_ssid('1.2.3.100'))
        self.assertEqual(self.SSID_2, utils.get_wireless_ssid('4.3.2.100'))
        self.assertEqual(self.DEFAULT_SSID,
                         utils.get_wireless_ssid('100.0.0.100'))


if __name__ == "__main__":
    unittest.main()
