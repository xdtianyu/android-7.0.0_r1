#!/usr/bin/python
#
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unittests for server/cros/dynamic_suite/frontend_wrappers.py"""

import mox
import unittest

import common

from autotest_lib.server.cros.dynamic_suite import frontend_wrappers


class FrontendWrappersTest(mox.MoxTestBase):
  """Unit tests for frontend_wrappers global functions."""

  def testConvertTimeoutToRetryBasic(self):
    """Test converting timeout and delay values to retry attempts."""
    backoff = 2
    timeout_min = 10
    delay_sec = 10

    max_retry = frontend_wrappers.convert_timeout_to_retry(backoff,
                                                           timeout_min,
                                                           delay_sec)

    self.assertEquals(max_retry, 6)

  def testConvertTimeoutToRetryLimit(self):
    """Test approaching a change in attempt amount."""
    backoff = 2
    delay_sec = 10
    timeout_min_lower_limit = 42.499999
    timeout_min_at_limit = 42.5
    timeout_min_upper_limit = 42.599999

    max_retry_lower_limit = frontend_wrappers.convert_timeout_to_retry(
        backoff, timeout_min_lower_limit, delay_sec)

    max_retry_at_limit = frontend_wrappers.convert_timeout_to_retry(
        backoff, timeout_min_at_limit, delay_sec)

    max_retry_upper_limit = frontend_wrappers.convert_timeout_to_retry(
        backoff, timeout_min_upper_limit, delay_sec)

    # Eight attempts with a backoff factor of two should be sufficient
    # for timeouts up to 2550 seconds (or 42.5 minutes).
    self.assertEquals(max_retry_lower_limit, 8)
    self.assertEquals(max_retry_at_limit, 8)

    # We expect to see nine attempts, as we are above the 42.5 minute
    # threshold.
    self.assertEquals(max_retry_upper_limit, 9)

  def testConvertTimeoutToRetrySmallTimeout(self):
    """Test converting to retry attempts when a small timeout is used."""
    backoff = 2
    timeout_min = 0.01
    delay_sec = 10

    max_retry = frontend_wrappers.convert_timeout_to_retry(backoff,
                                                           timeout_min,
                                                           delay_sec)

    # The number of attempts should be less than one using the formula
    # outlined in the function, but, we always round up to the nearest
    # integer.
    self.assertEquals(max_retry, 1)

  def testConvertTimeoutToRetrySmallDelay(self):
    """Test converting to retry attempts when the delay is small."""
    backoff = 2
    timeout_min = 30
    delay_sec = 0.01

    max_retry = frontend_wrappers.convert_timeout_to_retry(backoff,
                                                           timeout_min,
                                                           delay_sec)

    # The number of retries shouldn't be too large despite the small
    # delay as a result of backing off in an exponential fashion.
    self.assertEquals(max_retry, 18)


if __name__ == '__main__':
    unittest.main()
