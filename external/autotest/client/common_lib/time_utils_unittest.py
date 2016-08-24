# Copyright (c) 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import contextlib
import datetime
import os
import time
import unittest

import common

from autotest_lib.client.common_lib import time_utils


@contextlib.contextmanager
def set_time_zone(tz):
    """Temporarily set the timezone to the specified value.

    This is needed because the unittest can be run in a server not in PST.

    @param tz: Name of the timezone for test, e.g., US/Pacific
    """
    old_environ = os.environ.copy()
    try:
        os.environ['TZ'] = tz
        time.tzset()
        yield
    finally:
        os.environ.clear()
        os.environ.update(old_environ)
        time.tzset()


class time_utils_unittest(unittest.TestCase):
    """Unittest for time_utils function."""

    TIME_STRING = "2014-08-20 14:23:56"
    TIME_SECONDS = 1408569836
    TIME_OBJ = datetime.datetime(year=2014, month=8, day=20, hour=14,
                                 minute=23, second=56)

    def test_date_string_to_epoch_time(self):
        """Test date parsing in date_string_to_epoch_time()."""
        with set_time_zone('US/Pacific'):
            parsed_seconds = time_utils.date_string_to_epoch_time(
                    self.TIME_STRING)
            self.assertEqual(self.TIME_SECONDS, parsed_seconds)


    def test_epoch_time_to_date_string(self):
        """Test function epoch_time_to_date_string."""
        with set_time_zone('US/Pacific'):
            time_string = time_utils.epoch_time_to_date_string(
                    self.TIME_SECONDS)
            self.assertEqual(self.TIME_STRING, time_string)


    def test_to_epoch_time_success(self):
        """Test function to_epoch_time."""
        with set_time_zone('US/Pacific'):
            self.assertEqual(self.TIME_SECONDS,
                             time_utils.to_epoch_time(self.TIME_STRING))

            self.assertEqual(self.TIME_SECONDS,
                             time_utils.to_epoch_time(self.TIME_OBJ))


if __name__ == '__main__':
    unittest.main()
