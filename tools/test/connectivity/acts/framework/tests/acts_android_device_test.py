#!/usr/bin/env python3.4
#
#   Copyright 2016 - The Android Open Source Project
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

import logging
import mock
import os
import shutil
import tempfile
import unittest

from acts import base_test
from acts.controllers import android_device

# Mock log path for a test run.
MOCK_LOG_PATH = "/tmp/logs/MockTest/xx-xx-xx_xx-xx-xx/"
# The expected result of the cat adb operation.
MOCK_ADB_LOGCAT_CAT_RESULT = [
    "02-29 14:02:21.456  4454  Something\n",
    "02-29 14:02:21.789  4454  Something again\n"]
# A mockd piece of adb logcat output.
MOCK_ADB_LOGCAT = (
    "02-29 14:02:19.123  4454  Nothing\n"
    "%s"
    "02-29 14:02:22.123  4454  Something again and again\n"
    ) % ''.join(MOCK_ADB_LOGCAT_CAT_RESULT)
# Mock start and end time of the adb cat.
MOCK_ADB_LOGCAT_BEGIN_TIME = "02-29 14:02:20.123"
MOCK_ADB_LOGCAT_END_TIME = "02-29 14:02:22.000"

def get_mock_ads(num, logger=None):
    """Generates a list of mock AndroidDevice objects.

    The serial number of each device will be integer 0 through num - 1.

    Args:
        num: An integer that is the number of mock AndroidDevice objects to
            create.
    """
    ads = []
    for i in range(num):
        ad = mock.MagicMock(name="AndroidDevice",
                            logger=logger,
                            serial=i,
                            h_port=None)
        ads.append(ad)
    return ads

def get_mock_logger():
    return mock.MagicMock(name="Logger", log_path=MOCK_LOG_PATH)

def mock_get_all_instances(logger=None):
    return get_mock_ads(5, logger=logger)

def mock_list_adb_devices():
    return [ad.serial for ad in get_mock_ads(5)]

class MockAdbProxy():
    """Mock class that swaps out calls to adb with mock calls."""

    def __init__(self, serial):
        self.serial = serial

    def shell(self, params):
        if params == "id -u":
            return b"root"
        if (params == "getprop | grep ro.build.product" or
            params == "getprop | grep ro.product.name"):
            return b"[ro.build.product]: [FakeModel]"

    def bugreport(self, params):
        expected = os.path.join(MOCK_LOG_PATH,
                                "AndroidDevice%s" % self.serial,
                                "BugReports",
                                "test_something,sometime,%s.txt" % (
                                    self.serial))
        expected = " > %s" % expected
        assert params == expected, "Expected '%s', got '%s'." % (expected,
                                                                 params)

    def __getattr__(self, name):
        """All calls to the none-existent functions in adb proxy would
        simply return the adb command string.
        """
        def adb_call(*args):
            clean_name = name.replace('_', '-')
            arg_str = ' '.join(str(elem) for elem in args)
            return arg_str
        return adb_call

class ActsAndroidDeviceTest(unittest.TestCase):
    """This test class has unit tests for the implementation of everything
    under acts.controllers.android_device.
    """

    def setUp(self):
        """Creates a temp dir to be used by tests in this test class.
        """
        self.tmp_dir = tempfile.mkdtemp()

    def tearDown(self):
        """Removes the temp dir.
        """
        shutil.rmtree(self.tmp_dir)

    # Tests for android_device module functions.
    # These tests use mock AndroidDevice instances.

    @mock.patch.object(android_device, "get_all_instances",
                       new=mock_get_all_instances)
    @mock.patch.object(android_device, "list_adb_devices",
                       new=mock_list_adb_devices)
    def test_create_with_pickup_all(self):
        pick_all_token = android_device.ANDROID_DEVICE_PICK_ALL_TOKEN
        actual_ads = android_device.create(pick_all_token, logging)
        for actual, expected in zip(actual_ads, get_mock_ads(5)):
            self.assertEqual(actual.serial, expected.serial)

    def test_create_with_empty_config(self):
        expected_msg = android_device.ANDROID_DEVICE_EMPTY_CONFIG_MSG
        with self.assertRaisesRegexp(android_device.AndroidDeviceError,
                                     expected_msg):
            android_device.create([], logging)

    def test_create_with_not_list_config(self):
        expected_msg = android_device.ANDROID_DEVICE_NOT_LIST_CONFIG_MSG
        with self.assertRaisesRegexp(android_device.AndroidDeviceError,
                                     expected_msg):
            android_device.create("HAHA", logging)

    def test_get_device_success_with_serial(self):
        ads = get_mock_ads(5)
        expected_serial = 0
        ad = android_device.get_device(ads, serial=expected_serial)
        self.assertEqual(ad.serial, expected_serial)

    def test_get_device_success_with_serial_and_extra_field(self):
        ads = get_mock_ads(5)
        expected_serial = 1
        expected_h_port = 5555
        ads[1].h_port = expected_h_port
        ad = android_device.get_device(ads,
                                       serial=expected_serial,
                                       h_port=expected_h_port)
        self.assertEqual(ad.serial, expected_serial)
        self.assertEqual(ad.h_port, expected_h_port)

    def test_get_device_no_match(self):
        ads = get_mock_ads(5)
        expected_msg = ("Could not find a target device that matches condition"
                        ": {'serial': 5}.")
        with self.assertRaisesRegexp(android_device.AndroidDeviceError,
                                     expected_msg):
            ad = android_device.get_device(ads, serial=len(ads))

    def test_get_device_too_many_matches(self):
        ads = get_mock_ads(5)
        target_serial = ads[1].serial = ads[0].serial
        expected_msg = "More than one device matched: \[0, 0\]"
        with self.assertRaisesRegexp(android_device.AndroidDeviceError,
                                     expected_msg):
            ad = android_device.get_device(ads, serial=target_serial)

    # Tests for android_device.AndroidDevice class.
    # These tests mock out any interaction with the OS and real android device
    # in AndroidDeivce.

    @mock.patch('acts.controllers.adb.AdbProxy', return_value=MockAdbProxy(1))
    def test_AndroidDevice_instantiation(self, MockAdbProxy):
        """Verifies the AndroidDevice object's basic attributes are correctly
        set after instantiation.
        """
        mock_serial = 1
        ml = get_mock_logger()
        ad = android_device.AndroidDevice(serial=mock_serial, logger=ml)
        self.assertEqual(ad.serial, 1)
        self.assertEqual(ad.model, "fakemodel")
        self.assertIsNone(ad.adb_logcat_process)
        self.assertIsNone(ad.adb_logcat_file_path)
        expected_lp = os.path.join(ml.log_path,
                                   "AndroidDevice%s" % mock_serial)
        self.assertEqual(ad.log_path, expected_lp)

    @mock.patch('acts.controllers.adb.AdbProxy', return_value=MockAdbProxy(1))
    @mock.patch('acts.utils.create_dir')
    @mock.patch('acts.utils.exe_cmd')
    def test_AndroidDevice_take_bug_report(self,
                                           exe_mock,
                                           create_dir_mock,
                                           MockAdbProxy):
        """Verifies AndroidDevice.take_bug_report calls the correct adb command
        and writes the bugreport file to the correct path.
        """
        mock_serial = 1
        ml = get_mock_logger()
        ad = android_device.AndroidDevice(serial=mock_serial, logger=ml)
        ad.take_bug_report("test_something", "sometime")
        expected_path = os.path.join(MOCK_LOG_PATH,
                                     "AndroidDevice%s" % ad.serial,
                                     "BugReports")
        create_dir_mock.assert_called_with(expected_path)

    @mock.patch('acts.controllers.adb.AdbProxy', return_value=MockAdbProxy(1))
    @mock.patch('acts.utils.create_dir')
    @mock.patch('acts.utils.start_standing_subprocess', return_value="process")
    @mock.patch('acts.utils.stop_standing_subprocess')
    def test_AndroidDevice_take_logcat(self,
                                       stop_proc_mock,
                                       start_proc_mock,
                                       creat_dir_mock,
                                       MockAdbProxy):
        """Verifies the steps of collecting adb logcat on an AndroidDevice
        object, including various function calls and the expected behaviors of
        the calls.
        """
        mock_serial = 1
        ml = get_mock_logger()
        ad = android_device.AndroidDevice(serial=mock_serial, logger=ml)
        expected_msg = ("Android device .* does not have an ongoing adb logcat"
                        " collection.")
        # Expect error if stop is called before start.
        with self.assertRaisesRegexp(android_device.AndroidDeviceError,
                                     expected_msg):
            ad.stop_adb_logcat()
        ad.start_adb_logcat()
        # Verify start did the correct operations.
        self.assertTrue(ad.adb_logcat_process)
        expected_log_path = os.path.join(
                                    MOCK_LOG_PATH,
                                    "AndroidDevice%s" % ad.serial,
                                    "adblog,fakemodel,%s.txt" % ad.serial)
        creat_dir_mock.assert_called_with(os.path.dirname(expected_log_path))
        adb_cmd = 'adb -s %s logcat -v threadtime  >> %s'
        start_proc_mock.assert_called_with(adb_cmd % (ad.serial,
                                                      expected_log_path))
        self.assertEqual(ad.adb_logcat_file_path, expected_log_path)
        expected_msg = ("Android device .* already has an adb logcat thread "
                        "going on. Cannot start another one.")
        # Expect error if start is called back to back.
        with self.assertRaisesRegexp(android_device.AndroidDeviceError,
                                     expected_msg):
            ad.start_adb_logcat()
        # Verify stop did the correct operations.
        ad.stop_adb_logcat()
        stop_proc_mock.assert_called_with("process")
        self.assertIsNone(ad.adb_logcat_process)
        self.assertEqual(ad.adb_logcat_file_path, expected_log_path)

    @mock.patch('acts.controllers.adb.AdbProxy', return_value=MockAdbProxy(1))
    @mock.patch('acts.utils.start_standing_subprocess', return_value="process")
    @mock.patch('acts.utils.stop_standing_subprocess')
    @mock.patch('acts.logger.get_log_line_timestamp',
                return_value=MOCK_ADB_LOGCAT_END_TIME)
    def test_AndroidDevice_cat_adb_log(self,
                                       mock_timestamp_getter,
                                       stop_proc_mock,
                                       start_proc_mock,
                                       MockAdbProxy):
        """Verifies that AndroidDevice.cat_adb_log loads the correct adb log
        file, locates the correct adb log lines within the given time range,
        and writes the lines to the correct output file.
        """
        mock_serial = 1
        ml = get_mock_logger()
        ad = android_device.AndroidDevice(serial=mock_serial, logger=ml)
        # Expect error if attempted to cat adb log before starting adb logcat.
        expected_msg = ("Attempting to cat adb log when none has been "
                        "collected on Android device .*")
        with self.assertRaisesRegexp(android_device.AndroidDeviceError,
                                     expected_msg):
            ad.cat_adb_log("some_test", MOCK_ADB_LOGCAT_BEGIN_TIME)
        ad.start_adb_logcat()
        # Direct the log path of the ad to a temp dir to avoid racing.
        ad.log_path = os.path.join(self.tmp_dir, ad.log_path)
        mock_adb_log_path = os.path.join(ad.log_path, "adblog,%s,%s.txt" %
                                         (ad.model, ad.serial))
        with open(mock_adb_log_path, 'w') as f:
            f.write(MOCK_ADB_LOGCAT)
        ad.cat_adb_log("some_test", MOCK_ADB_LOGCAT_BEGIN_TIME)
        cat_file_path = os.path.join(ad.log_path,
                                     "AdbLogExcerpts",
                                     ("some_test,02-29 14:02:20.123,%s,%s.txt"
                                     ) % (ad.model, ad.serial))
        with open(cat_file_path, 'r') as f:
            actual_cat = f.read()
        self.assertEqual(actual_cat, ''.join(MOCK_ADB_LOGCAT_CAT_RESULT))
        # Stops adb logcat.
        ad.stop_adb_logcat()

if __name__ == "__main__":
   unittest.main()